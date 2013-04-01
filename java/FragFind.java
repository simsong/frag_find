/**
 * Java FragFind class to match the C++ class that we have.
 */

import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.FileChannel.MapMode;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.*;

/** Blocklist is a list of block numbers */
class Blocklist extends Vector<Long> {
}

/** A FileMap is a vector of block lists.
 * The  key is the block number of the mapped file, so a file that is 64k in length
 * would have 128 entries. Each element is a blocklist of all the locations
 * on the disk image where the block appears.
 */
class FileMap extends Vector<Blocklist> {
    /* Clean the FileMap. If any sector in the input file is in more than
     * one place in the image, check to see if the previous next is in just one place.
     * If so, then only pick the sector that aligns.
     */
    public void clean(){
	for(int b=1;b<size();b++){
	    if(get(b-1).size()==1 && get(b).size()>1){
		long predicted = get(b-1).firstElement()+1;
		if(get(b).contains(predicted)){
		    get(b).clear();
		    get(b).add(predicted);
		}
	    }
	}
	

	/* Clean in the reverse direction */
	for(int b=size()-2;b>=0;b--){
	    if(get(b+1).size()==1 && get(b).size()>1){
		long predicted = get(b+1).firstElement()-1;
		if(get(b).contains(predicted)){
		    get(b).clear();
		    get(b).add(predicted);
		}
	    }
	}
    }

    /* Return the length of a given mapping run --- the number of sequential blocks
     * starting at a given location.
     */
    int runlen(int target_block,long image_block){
	int len = 1;
	for(len = 1;target_block + len < size(); len++){
	    if( get(target_block+len).contains(image_block+len)==false){
		return len;
	    }
	}
	return len;
    }

    int found_count(){
	int count = 0;
	for(int b=0;b<(long)size();b++){
	    if (get(b).size()>0) count++;
	}
	return count;
    }

    double found_percent(){
	return found_count() * 100.0 / size();
    }

    void print(){
	System.out.printf("Target Block(s)     Found at image block\n");
	for(int tb=0;tb<size();){
	    if (get(tb).size()==0){
		tb++;			// target block not in image
		continue;
	    }
	    long first_ib = get(tb).firstElement();
	    long runlen = runlen(tb,first_ib);
	    if(runlen>1){
		System.out.printf("%10d-%-10d    %10d-%-10d   (%d blocks)\n",
		       tb,tb+runlen-1,
		       first_ib,
		       first_ib+runlen-1,
		       runlen);
		tb += runlen;
		continue;
	    }
	    boolean first=true;
	    for(Iterator<Long> i = get(tb).iterator(); i.hasNext(); ){
		Long bl = i.next();
		if(first){
		    System.out.printf("%10d               ",tb);
		    first=false;
		}
		System.out.printf(" %10d ",bl);
	    }
	    if(first==false) System.out.printf("\n");
	    tb++;
	}
    }
}

/** 
 * Blockfile is a simple abstraction that lets you read a file block-by-block
 */
class Blockfile {
    ImageFile	f;
    long		block_at_buffer0;
    int			bytes_in_buffer;
    int			blocksize;
    String		filename;
    long		filesize;
    long		blocks;
    FileChannel		vectorChannel;

    public void open(String fname,int blocksize) throws FileNotFoundException,IOException {
	this.filename   = fname;
	this.blocksize  = blocksize;

	f = new ImageFile(fname,"r");
	this.filesize   = f.totalSectors * f.sectorSize;
	this.blocks	= filesize/blocksize;
    }
    byte[] getblock(long blocknumber) throws IOException{
	f.seek(blocknumber*blocksize);
	byte[] buf = new byte[512];
	f.read(buf);
	return buf;
    }
}

/** sha1map maps individual sha1's to a list of blocks.
 * It has to be a lst of blocks, because multiple blocks may have the same SHA1.
 */

class Sha1Map extends HashMap<Sha1,Blocklist> {
};



class Targetfile extends Blockfile {
    Sha1Map sha1map = new Sha1Map();			// where each SHA1 is in the targetfile
    FileMap filemap = new FileMap();			// where each block is in the imagemap
    void load(String fname,Prefilter prefilter) throws IOException{
	super.open(fname,512);
	System.out.printf("Load %s Blocks: %s %d-byte blocks\n",fname,blocks,blocksize);

	/** Read through the target file block by block and create a SHA1 structure
	 * for each block that is read. Each SHA1 must be allocated because we need a place
	 * to keep all of these SHA1s. The map points to each SHA1.
	 */
	for(int blocknumber=0;blocknumber < blocks; blocknumber++){
	    byte[] buf = getblock(blocknumber);

	    prefilter.set_from_buf(buf);
	    Sha1 sha1 = new Sha1(buf);

	    // add to the map and to the bloom filter
	    if(sha1map.containsKey(sha1)==false){
		sha1map.put(sha1,new Blocklist());
	    }

	    sha1map.get(sha1).add((long)blocknumber);
	    //TK b.add(sha1->sha1);
	}
	filemap.setSize((int)blocks);
    }
};

class Targets extends Vector<Targetfile> {
};



/** The prefilter */

class Prefilter extends BitSet {
    int max = 16777216;
    BitSet bs = new BitSet(max);

    /** return a very fast-to-calculate 24-bit value from a 512-byte buffer*/
    static int val3char(byte[] buf) {
	java.util.zip.Adler32 a32 = new java.util.zip.Adler32();
	a32.update(buf);
	return (int)a32.getValue() & 0xffffff;
    }
    long hits = 0;
    double utilization() {
	return (double)bs.cardinality() / 16777216.0;
    }
    void set_from_buf(byte[] buf){ bs.set(val3char(buf)); }
    boolean query_from_buf(byte[] buf){ return bs.get(val3char(buf)); }
};

public class FragFind {
    public static void main(String[] args){
	if(args.length<2){
	    System.out.println("usage: FragFind <diskimage> <file1> [file2 ...]");
	    return;
	}
	FragFind ff = new FragFind();
	try{
	    for(int i=1;i<args.length;i++){
		ff.loadTarget(args[i]);
	    }
	    ff.searchVolume(args[0]);
	    ff.printReport();
	} catch (FileNotFoundException e){
	    e.printStackTrace();
	} catch (IOException e){
	    e.printStackTrace();
	}
    }
    Targets targets = new Targets();
    Prefilter prefilter = new Prefilter();
    public void loadTarget(String fname) throws FileNotFoundException,IOException{
	Targetfile t = new Targetfile();
	t.load(fname,prefilter);
	targets.add(t);
    }
    public void searchVolume(String fname) throws FileNotFoundException,IOException{
	Blockfile imagefile = new Blockfile();
	imagefile.open(fname,512);
	for(long blocknumber=0;blocknumber < imagefile.blocks;blocknumber++){
	    byte[] buf = imagefile.getblock(blocknumber);

	    /* Is it in the prefilter? */
	    if(prefilter.query_from_buf(buf)==false){
		continue;
	    }

	    Sha1 sha1 = new Sha1(buf);	// get the sha1

	    //TK: check bloom filter
	    /* Get the list of blocks where the SHA1 is observed and add to each filemap */
	    for(Iterator<Targetfile> ti = targets.iterator(); ti.hasNext(); ){
		Targetfile t = ti.next();
		if(t.sha1map.containsKey(sha1)==false){ // not in it
		    continue;
		}
		for(Iterator<Long> ii = t.sha1map.get(sha1).iterator(); ii.hasNext();){
		    int i = ii.next().intValue();
		    if(t.filemap.get(i)==null) t.filemap.set(i,new Blocklist());
		    t.filemap.get(i).add(blocknumber);
		}
	    }
	}
    }
    public void printReport(){
	for(Iterator<Targetfile> ti = targets.iterator(); ti.hasNext(); ){
	    Targetfile t = ti.next();
	    t.filemap.clean();
	    t.filemap.print();
	}
    }
}


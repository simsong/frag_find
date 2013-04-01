/**
 * Java NSRLBloom class to match the C++ class that we have.
 */

import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.FileChannel.MapMode;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class NSRLBloom {
    int hash_bits;
    int hash_bytes;
    int M;
    int k;
    int vector_offset;
    ByteBuffer vector;
    String comment;
    int added_items;
    ByteBuffer key;
    MessageDigest md;
    int debug=0;

    NSRLBloom(String fname,int hash_size,int M,int k,String comment){
	this.M = M;
	this.k = k;
	this.comment = comment;
	if(fname==null){
	    int vector_bytes = 1<<(M-3);
	    System.out.println("Vector_bytes:"+vector_bytes);
	    vector = ByteBuffer.wrap(new byte[vector_bytes]);
	}
	try {
	    if(hash_size==128) md = MessageDigest.getInstance("SHA-1");
	    if(hash_size==160) md = MessageDigest.getInstance("SHA-1");
	} catch (NoSuchAlgorithmException e) {
	    e.printStackTrace();
	}
	System.out.println("vector.length="+vector.limit()+" M="+M+" k="+k);
    }

    void open(String fname,int mode) throws java.io.IOException {
	BufferedReader br = new BufferedReader(new FileReader(fname));
	String s;
	while((s=br.readLine())!=null){
	    System.out.println(s);
	}
	br = null;
	FileChannel vectorChannel = new FileInputStream(fname).getChannel();
	vector = vectorChannel.map(MapMode.READ_ONLY,0,vectorChannel.size());
    }

    void set_bloom_bit(int bit) {
	byte q = (byte) (((byte)1)<<(bit % 8));
	int pos = bit/8;
	if(debug>1) System.out.println("set_bloom_bit("+bit+") q="+q+" pos="+pos);
	vector.put(pos,(byte) (vector.get(pos) | q));
    }

    int get_bloom_bit(int bit){
	byte q = (byte) (((byte)1)<<(bit % 8));
	int pos = bit/8;
	if(debug>1) System.out.println("get_bloom_bit("+bit+")");
	return (vector.get(pos) & q) == 0 ? 0 : 1;
    }

    void bloom_info_update(){
    }

    static int get_bit(final byte[] buf,int bit){
	return (buf[bit/8] & (1<<bit%8))==0 ? 0 : 1;
    }

    void add(byte[] hash){
	for(int i=0;i<k;i++){
	    int offset = i * M;
	    int v = 0;
	    for(int j=0;j<M;j++){
		v = (v<<1) | get_bit(hash,offset+j);
	    }
	    set_bloom_bit(v);
	}
	added_items++;
	if (added_items % 1000==0) bloom_info_update();
    }
    
    byte[] hashForString(String s){
	try {
	    MessageDigest md2 = (MessageDigest) md.clone();
	    md2.update(s.getBytes(), 0, s.length());
	    return md2.digest();
	} catch (CloneNotSupportedException e) {
	    return null;
	} 
    }

    void add(String s){
	add(hashForString(s));
    }

    boolean query(byte[] hash){
	for(int i=0;i<k;i++){
	    int offset = i * M;
	    int v = 0;
	    for(int j=0;j<M;j++){
		v = (v<<1) | get_bit(hash,offset+j);
	    }
	    if(get_bloom_bit(v)==0) return false;
	}
	return true;
    }

    boolean query(String s){
	return query(hashForString(s));
    }

    
    public static void main(String[] args){
	System.out.println("start");

	NSRLBloom b = new NSRLBloom(null,160,16,4,"This is a comment");
	/* Set the numbers 1 through 100 */
	for(int i=0;i<100;i+=2){
	    String s = "Item "+i;
	    b.add(s);
	}
	System.out.println("Querying what's there...");
	for(int i=0;i<100;i++){
	    String s = "Item "+i;
	    if(b.query(s) != (i%2==0)){
		System.out.println("Error. i="+i+" s="+s+" query="+b.query(s));
	    }
	    b.add(s);
	}
    }
}

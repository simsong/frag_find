/*
 * frag_find: given a file, find all of its fragments on the hard drive.
 * Automatically ignore popular fragments. 
 *
 * Performance Log:
 * Version 1.1.1 - 20724 MD5 calcuations
 */

#include "config.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#ifdef HAVE_ERR_H
#include <err.h>
#endif

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <cstring>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

#include <map>
#include <vector>
#include <bitset>
#include <iostream>
#include <fstream>

#ifndef DEFAULT_BLOCKSIZE
#define DEFAULT_BLOCKSIZE 512
#endif

#include "bloom.h"
#include "aftimer.h"
//#include "hash_t.h"
#include "md5.h"

//#include "utils.h"
#include "blockfile.h"
#include "dfxml.h"
#include "xml.h"
#include "myregex.h"

using namespace std;

string _e = string("");

void *initial_break = 0;

blockfile imagefile;			// the file we are reading
bool use_prefilter = 0;
int opt_raw = 0;


inline int64_t stoi64(std::string s){
    return atoi64(s.c_str());
}


#ifndef HAVE_ERR
#include <stdarg.h>
// noreturn attribute to avoid warning with GCC on Linux
static void err(int eval,const char *fmt,...) __attribute__ ((noreturn));
static void err(int eval,const char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
  fprintf(stderr,": %s\n",strerror(errno));
  exit(eval);
}
#endif

#ifndef HAVE_ERRX
#include <stdarg.h>
// noreturn attribute to avoid warning with GCC on Linux
static void errx(int eval,const char *fmt,...) __attribute__ ((noreturn));
static void errx(int eval,const char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  vfprintf(stderr,fmt,ap);
  fprintf(stderr,"%s\n",strerror(errno));
  va_end(ap);
  exit(eval);
}
#endif




/** blocklist is a list of block numbers.
 */

class blocklist_t:public vector<int64_t> {
public:
    /** return true if the blocklist contains a given block number */
    bool contains(int64_t block) const {
	for(blocklist_t::const_iterator i = this->begin(); i != this->end(); i++){

	    if(*i == block) return true;
	}
	return false;
    }
};

/* A filemap is a vector of blocklists.
 * The key is the block number of the mapped file, so a file that is 64k in length
 * would have 128 entries. Each element is a blocklist of all the locations
 * on the disk image where the block appears.
 */
class filemap_t:public vector<blocklist_t> {
public:
    /* Clean the filemap. If any sector in the input file is in more than
     * one place in the image, check to see if the previous next is in just one place.
     * If so, then only pick the sector that aligns.
     */
    void clean(){
	for(int64_t b=1;b<(int64_t)this->size();b++){
	    if((*this)[b-1].size()==1 && (*this)[b].size()>1){
		uint64_t predicted = (*this)[b-1].front()+1;
		if((*this)[b].contains(predicted)){
		    (*this)[b].clear();
		    (*this)[b].push_back(predicted);
		}
	    }
	}
	
	/* Clean in the reverse direction */
	for(int64_t b=this->size()-2;b>=0;b--){
	    if((*this)[b+1].size()==1 && (*this)[b].size()>1){
		uint64_t predicted = (*this)[b+1].front()-1;
		if((*this)[b].contains(predicted)){
		    (*this)[b].clear();
		    (*this)[b].push_back(predicted);
		}
	    }
	}
    }

    /* Return the length of a given mapping run --- the number of sequential blocks
     * starting at a given location.
     */
    uint64_t runlen(uint64_t master_block,uint64_t image_block){
	uint64_t len = 1;
	for(len = 1;master_block + len < (uint64_t)this->size(); len++){
	    if(! (*this)[master_block+len].contains(image_block+len)){
		return len;
	    }
	}
	return len;
    }

    uint64_t found_count(){
	uint64_t count = 0;
	for(uint64_t b=0;b<(uint64_t)this->size();b++){
	    if ((*this)[b].size()>0) count++;
	}
	return count;
    }

    double found_percent(){
	return found_count() * 100.0 / this->size();
    }
};



/** md5map maps individual md5's to a list of blocks.
 * It has to be a lst of blocks, because multiple blocks may have the same MD5.
 */

class md5map_t:public map<md5_t,blocklist_t> {
};

/**
 * The prefilter. - allow blocks to be added and then report if they are present or not.
 */

class prefilter_t {
public:
    uint64_t hits;
    prefilter_t():hits(0){
    }
    unsigned int buf2hash(const unsigned char *buf){
	return (buf[100]<<16) | (buf[101]<<8) | (buf[102]);
    }

    bool operator[](unsigned int) {
	return false;
    }
    double utilization() {
	uint64_t seen = 0;
	for(int i=0;i<16777216;i++){
	    if((*this)[i]) seen++;
	}
	return (double)seen / 16777216.0;
    }
    void set_from_buf(const u_char *buf){
    }
    bool query_from_buf(const u_char *buf){
	bool ret = (*this)[buf2hash(buf)];
	if(ret) hits++;
	return ret;
    }
};


int opt_M = 32;
void usage()
{
    printf("frag_find version "PACKAGE_VERSION"\n");
    printf("usage: frag_find [options] <raw disk file> [-m md5deep-output.txt | <master1> ] [<master2...>] \n");
    printf("\n");
    printf("options:\n");
    printf("     -s <start>   - start image scan at block <start> (default is beginning)\n");
    printf("     -e <end>     - end image scan at block <start> (default is end)\n");
    printf("     -b blocksize - sets the blocksize (default is %d)\n",DEFAULT_BLOCKSIZE);
    printf("     -m md5deep-output.txt - read a set of piecewise hashes from md5deep\n");
    printf("     -r           - prints the raw association map, in addition to the cleaned one\n");
    printf("     -S           - print stats (takes a LONG time)\n");
    printf("     -MN          - specifies size of bloom filter, in 2^N bits (default %d)\n",opt_M);
    printf("     -X1          - do not use bloom filter or prefilter\n");
    printf("     -X2          - do not use prefilter\n");
    printf("     -xfname.xml  - output digital forensics XML file to fname.xml\n");
    exit(1);
}

/* A masterfile is like a blockfile, but it also keeps track of:
 * -  a md5map, which is the block numbers in the master where each md5 exists.
 * - a filemap, an array that holds all of the block lists
 */
class masterfile:public blockfile {
public:;
    md5_t    md5;			// the file's hash code
    md5map_t md5map;
    filemap_t filemap;
    void add_block_hash(uint32_t blocknumber,const md5_t &md5,NSRLBloom &b);
    void load(const char *fname,NSRLBloom &b, uint32_t blocksize);
    void print_report(class xml *, uint32_t blocksize);
};

void masterfile::print_report(class xml *x, uint32_t blocksize) {
    printf("Master Block(s)     Found at image block\n");
    for(uint64_t tb=0;tb<(uint64_t)filemap.size();){
	if (filemap[tb].size()==0){
	    tb++;			// master block not in image
	    continue;
	}
	uint64_t first_ib = filemap[tb].front();
	uint64_t run_blocks = filemap.runlen(tb,first_ib);
	uint64_t run_bytes = run_blocks * blocksize;

	/* If run_bytes extends past the end of the file, shorten it to the file length */
	if(tb*blocksize + run_bytes > filesize){
	    run_bytes = filesize - tb*blocksize;
	}

	if(run_blocks>1){
	    printf("%10"PRId64"-%-10"PRId64"    %10"PRId64"-%-10"PRId64"   (%"PRId64" blocks)\n",
		   tb,tb+run_blocks-1, first_ib, first_ib+run_blocks-1, run_blocks);
	    if(x){
		char buf[1024];
		snprintf(buf,sizeof(buf),"file_offset='%"PRId64"' img_offset='%"PRId64"' len='%"PRId64"'",
			 tb*blocksize,first_ib*blocksize,run_bytes);
		x->xmlout(string("run"),_e,string(buf),false);
	    }
	    tb += run_blocks;
	    continue;
	}
	bool first=true;
	for(blocklist_t::const_iterator i = filemap[tb].begin();
	    i != filemap[tb].end();
	    i++){
	    if(first){
		printf("%10"PRId64"               ",tb);
		first = false;
	    }
	    printf(" %10"PRId64" ",*i);
	    if(x){
		char buf[1024];
		snprintf(buf,sizeof(buf),"file_offset='%"PRId64"' img_offset='%"PRId64"' len='%"PRIu64"'",
			 tb*blocksize,(*i) * blocksize,run_bytes);
		x->xmlout(string("run"),_e,string(buf),false);
	    }
	}
	if(first==false) printf("\n");
	tb++;
    }

    /* Now see if the partial block at the end of any of the possible runs match */
    int extra_bytes = filesize % blocksize;
    if(extra_bytes){
	for(blocklist_t::const_iterator i=filemap.back().begin();
	    i!=filemap.back().end();
	    i++){

	    bool done = false;
	    uint64_t last_image_block  = *i;
	    uint64_t last_master_block = filemap.size()-1;

	    /* See if the next block has the data... */
	    u_char *image_extra = (u_char *)calloc(extra_bytes,1);
	    u_char *master_extra = (u_char *)calloc(extra_bytes,1);
	    
	    ssize_t ilen = imagefile.pread((void *)image_extra,extra_bytes,(last_image_block+1)*blocksize);
	    ssize_t tlen = this->pread((void *)master_extra,extra_bytes,(last_master_block+1)*blocksize);
	    if(tlen==extra_bytes && ilen==extra_bytes && memcmp(master_extra,image_extra,extra_bytes)==0){
		if(x){
		    char buf[1024];
		    snprintf(buf,sizeof(buf),"file_offset='%"PRId64"' img_offset='%"PRId64"' len='%d'",
			     (last_master_block+1)*blocksize,
			     (last_image_block+1)*blocksize,
			     extra_bytes);
		    x->xmlout(string("run"),_e,string(buf),false);
		}
		done=true;
	    }
	    free(image_extra);
	    free(master_extra);
	    if(done) break;
	}
    }
}


/*
 * Read every block of the master file and add each to the md5 maps.
 */
void masterfile::add_block_hash(uint32_t blocknumber,const md5_t &md5,NSRLBloom &b)
{
    // report if it is in the input file twice
    if(opt_raw && b.query(md5.digest)){
	cout << "Input file block " << blocknumber << "appears previously at ";;
	for(blocklist_t::const_iterator i = this->md5map[md5].begin();
	    i!=this->md5map[md5].end();
	    i++){
	    printf("%"PRId64" ",*i);
	}
	printf("\n");
    }
    
    // add to the map and to the bloom filter
    this->md5map[md5].push_back(blocknumber);
    b.add(md5.digest);
}

void masterfile::load(const char *fname,NSRLBloom &b, uint32_t blocksize)
{
    if(this->open(fname,blocksize)<0){
	err(1,"Error: Cannot open %s",fname);
    }
	
    if(imagefile.filesize < this->filesize ){
	err(1,"Error: Master file %s is larger than image file; cannot continue\n",
	    fname);
    }
	    
    cout << fname << "\n";
    printf("Blocks: %"PRId64" (%d-byte blocks)\n", this->blocks,blocksize);
    md5 = md5_generator::hash_file(fname);
    cout << "MD5:  " <<md5.hexdigest() << "\n";

    /** Read through the master file block by block and create a MD5 structure
     * for each block that is read. Each MD5 must be allocated because we need a place
     * to keep all of these MD5s. The map points to each MD5.
     */
    u_char *buf = (u_char *)malloc(blocksize);
    for(uint64_t blocknumber=0;blocknumber < this->blocks; blocknumber++){
	if(this->getblock(blocknumber,buf) <0) break;
	//if(use_prefilter) prefilter.set_from_buf(buf);
	const md5_t md5 = md5_generator::hash_buf(buf,blocksize);
	add_block_hash(blocknumber,md5,b);
    }
    free(buf);
    this->filemap.resize(this->blocks);
}

class masters_t:public vector<masterfile *>{
private:
    myregex reg;
    int md5deep_parse(string line,string *md5hex,string *fname,int64_t *start,int64_t *len);
public:;
    masters_t():reg("([0-9a-f]{32})  (.*) offset ([0-9]+)-([0-9]+)",0){};
    NSRLBloom b;			  // bloom prefilter
    void bloom_create(int opt_M);	  // initialize the bloom filter
    void add_master_file(const char *fn, uint32_t blocksize); // piecewise hash a file
    void read_md5deep(const char *fn, uint32_t blocksize);	// read a set of files
};


void masters_t::bloom_create(int opt_M)
{
    printf("Creating bloom filter...\n");
    if(b.create(0,128,opt_M,4,"In-memory bloom filter for frag find")){
	printf(">>> not enough memory for bloom filter\n");
	exit(1);
    }
    printf("Bloom filter created.\n");
}

void masters_t::add_master_file(const char *fn, uint32_t blocksize)
{
    printf("Adding Master File: %s (file #%zd)\n",fn,this->size()+1);
    masterfile *t = new masterfile();
    t->load(fn,b,blocksize);
    this->push_back(t);
}

/*
 * Attempt to parse a line from md5deep. Return 0 if success, -1 if failure.
 */
int masters_t::md5deep_parse(string line,string *md5hex,string *fname,int64_t *start,int64_t *len)
{
    string matches[4];

    reg.search(line,matches,4);
    if(matches[0].size()!=32 || matches[1].size()==0 || matches[2].size()==0 || matches[3].size()==0) return -1;
    *md5hex = matches[0];
    *fname  = matches[1];
    *start  = stoi64(matches[2]);
    int64_t end    = stoi64(matches[3]);
    *len    = end - *start;
    for(int i=0;i<4;i++){
	cout << ">matches[" << i << "]=" << matches[i] << "\n";
    }
    return 0;
}

void masters_t::read_md5deep(const char *fn, uint32_t blocksize)
{
    ifstream f(fn);
    if(!f.is_open()){
	cerr << "Cannot open " << fn << ": " << strerror(errno) << "\n";
	exit(1);
    }
    //bool   first = true;
    //uint64_t ignored_hashes = 0;
    //uint64_t ingested_hashes = 0;

    string md5hex_last;
    string fname_last;
    //int64_t start_last;
    //int64_t len_last = 0;

    while(!f.eof()){
	string line;
	string md5hex,fname;
	int64_t start=0,len=0;
	getline(f,line);

	/* Read the next line and see if it is valid */

	if(md5deep_parse(line,&md5hex,&fname,&start,&len)){
	    cerr << "Error: " << fn << " does not appear to be an md5deep-foramtted file\n";
	    cerr << "Please create a piecewise hash file with a command such as this:\n";
	    cerr << "\n";
	    cerr << "   md5deep -p512 -r masterfiles/\n";
	    cerr << "\n";
	    cerr << "Your file contains this line:\n";
	    cerr << line << "\n";
	    cerr << "\n";
	    cerr << "You want a file that begins with a line that looks like this:\n";
	    cerr << "8cfc37aee1b2fcb6843dafa982cc5567  back.jpg offset 0-511\n";
	    exit(1);
	}

#if 0
	/* Now process the previous line */
	if(!first){

	    /* Check for incorrect blocksize in any line of the file but the last line */
	    if(fname_last == fname && len!=blocksize){
	    }
	}
		
#endif

	if(len > blocksize){
	    cerr << "Error: " << fn << " is a piecewise hash file with the wrong blocksize.\n";
	    cerr << "Expected block size: " << blocksize << "\n";
	    cerr << "Block size found: " << len << "\n";
	}
	
    }
}


int main(int argc,char **argv)
{
    masters_t masters;
    uint64_t opt_start = 0;
    uint64_t opt_end   = 0;
    int opt_stats = 0;
    int ch;
    bool use_bloom = 1;
    uint64_t bloom_false_positives=0;
    class xml *x = 0;
    string command_line;
    uint32_t blocksize = DEFAULT_BLOCKSIZE;
    string opt_md5;

    /* Make a copy of the command line */
    for(int i=0;i<argc;i++){
	if(i>0) command_line.push_back(' ');
	command_line.append(argv[i]);
    }

    prefilter_t prefilter;	// bitset to hold first 3 bytes of block

    while ((ch = getopt(argc,argv,"b:e:hM:m:Ss:rx:X:?")) != -1){
	switch(ch){
	case 's': opt_start = atoi64(optarg);break;
	case 'e': opt_end   = atoi64(optarg);break;
	case 'r': opt_raw   = 1;break;
	case 'b': blocksize = atoi(optarg); break;
	case 'S': opt_stats++;break;
	case 'M': opt_M     = atoi(optarg); break;
	case 'm': opt_md5 = optarg; break;
	case 'X':
	    switch(atoi(optarg)){
	    case 1: use_bloom = 0;break;
	    case 2: use_prefilter = 0;break;
		break;
	    default: fprintf(stderr,"Invalid -X setting\n");
		usage();
	    }
	    break;
	case 'x':
	    x = new xml(optarg,true);
	    break;
	case 'h':
	case '?':
	default:
	    usage();
	}
    }
    if(opt_M <5 || opt_M > 32){
	fprintf(stderr,"M must be between 5 and 32.\n");
	exit(1);
    }
    if (!opt_md5.empty()) masters.read_md5deep(opt_md5.c_str(),blocksize);

    argc -= optind; 
    argv += optind;
    
    if(argc<2) usage();

    if(blocksize<4){
	printf("Blocksizes smaller than 4 are meaningless. Try again.\n");
	exit(1);
    }

    masters.bloom_create(opt_M);

    if(imagefile.open(*argv,blocksize)<0) err(1,"Cannot open %s",imagefile.filename);
    if(imagefile.filesize==0){
	err(1,"Error: Image size %s is zero.\n",*argv);
    }

    if(imagefile.filesize < blocksize){
	err(1,"Image size (%"PRId64") is smaller than blocksize (%d); cannot continue.\n",
	    imagefile.filesize,blocksize);
    }

    argv++;

    /* Load up all of the master files */
    for(;*argv;argv++){
	masters.add_master_file(*argv,blocksize);
    }

    printf("Now searching image file...\n");
    uint64_t hits = 0;

    /**
     * Read through the image file block by block. This time we can use a shared MD5 structure
     * because we do not need to allocate a hash for each block.
     */

    if(opt_end==0) opt_end = imagefile.blocks;
    aftimer timer;
    timer.start();
    u_char *buf = (u_char *)malloc(blocksize);
    for(uint64_t blocknumber=opt_start;blocknumber < opt_end && blocknumber < imagefile.blocks; blocknumber++){
	/* If this is one of the 100,000 even blocks, print status info */
	if(blocknumber>opt_start && (((blocknumber-opt_start) % 100000)==0)){
	    uint64_t blocks = blocknumber-opt_start;
	    uint64_t total = opt_end-opt_start;
	    putchar('\r');
	    printf("%"PRId64"M out of %"PRId64"M sectors processed; hits=%"PRId64" ",
		   blocks/1000000,total/1000000,hits);
	    if(timer.elapsed_seconds()>1.0){
		uint64_t kblocks = blocks/1000;

		printf("; %3g Kblocks/sec; done in %s",
		       kblocks/timer.elapsed_seconds(),
		       timer.eta_text((double)blocks/(double)total).c_str());
	    }
	    fflush(stdout);
	}

	/* Scan through the input file block-by-block*/
	if(imagefile.getblock(blocknumber,buf)<0){
	    printf("premature end of file?\n");
	    break;
	}

	/* If block is not in prefilter, then there is no sense to compute hash */
	if(use_prefilter){
	    if(prefilter.query_from_buf(buf)==0){
		continue;
	    }
	}
	/* Compute the MD5 */
	md5_t md5 = md5_generator::hash_buf(buf,blocksize);

	/* If the block's MD5 is not in the bloom filter,
	 * there is no need to scan for it in the file maps.
	 */

	if(masters.b.query(md5.digest)==0) continue;

	/* Get the list of blocks where that MD5 was observed and add each one to the filemap.
	 */
	if(opt_raw && use_bloom) printf("Block %"PRId64" MD5 in bloom filter.\n",blocknumber);
	if(opt_raw) printf("   >>> found at ");
	uint64_t added = 0;

	for(masters_t::iterator t = masters.begin(); t!=masters.end(); t++){
	    for(blocklist_t::iterator i = (*t)->md5map[md5].begin();
		i!=(*t)->md5map[md5].end();
		i++){
		if(opt_raw) printf("   %"PRId64" ",*i);
		(*t)->filemap[*i].push_back(blocknumber);
		added++;
	    }
	}

	if(opt_raw) printf("\n");
	if(added==0) bloom_false_positives++;
	hits++;
    }
    free(buf);


    printf("\n\n");
    printf("Image file:  %s  (%"PRId64" blocks)\n",imagefile.filename,imagefile.blocks);
    printf("Blocksize: %d\n",masters[0]->blocksize);

    /* If we got this far, it's time to make the XML file */
    if(x){
	x->push("frag_find","xmloutputversion='0.3'");
	x->push("metadata",
		"\n  xmlns='http://afflib.org/frag_find/' "
		"\n  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
		"\n  xmlns:dc='http://purl.org/dc/elements/1.1/'" );
	x->xmlout("dc:type","Hash-Based Carving Report","",false);
	x->pop();

	/* Output carver information per photorec standard */
	x->push("creator");
	x->xmlout("program","frag_find");
	x->xmlout("version",PACKAGE_VERSION);
	x->add_DFXML_build_environment();
	x->add_DFXML_execution_environment(command_line);
	x->pop();			// creator
	x->push("source");
	x->xmlout("image_filename",imagefile.filename);
	x->xmlprintf("blocks","","%"PRIu64,imagefile.blocks);
	x->xmlprintf("blocksize","","%"PRIu32,(uint32_t)masters[0]->blocksize);
    }


    for(masters_t::iterator t = masters.begin(); t!=masters.end(); t++){
	printf("-------------------\n");
	printf("Master file: %s  (%"PRId64" blocks)\n",(*t)->filename,(*t)->blocks);
	if(x){
	    x->push("fileobject");
	    x->xmlout("filename",(*t)->filename);
	    x->xmlprintf("blocks",_e,"%"PRIu64,(*t)->blocks);
	    x->xmlprintf("filesize",_e,"%"PRIu64,(*t)->filesize);
	    x->xmlout("hashdigest",(*t)->md5.hexdigest(),string("type='md5'"),false);
	}
	if(opt_start!=0){
	    printf("   NOTE: Image scan started at block %"PRId64"\n",opt_start);
	}
	if(opt_end!=imagefile.blocks){
	    printf("   NOTE: Image scan ended at block %"PRId64"\n",opt_end);
	}

	printf("Blocks of master file found in image file: %"PRId64"\n",hits);
	printf("Here is where they were found:\n");

	if(opt_raw){
	    printf("RAW FILEMAP:\n");
	    printf("===============================================================================\n");
	    printf("\n");
	    (*t)->print_report(0,blocksize);
	}

	(*t)->filemap.clean();

	/* note: perhaps it would make more sense to iteratively clean. */
	(*t)->print_report(x,blocksize);
	printf("Total blocks of original file found: %"PRId64" (%2.0f%%)\n",
	       (*t)->filemap.found_count(),
	       (*t)->filemap.found_percent());
	if(opt_stats){
	    double runtime = timer.elapsed_seconds();
	    printf("Total number of prefilter hits: %"PRId64" (%"PRId64" blocks filtered out)\n",
		   prefilter.hits,(imagefile.blocks-opt_start)-prefilter.hits);
	    printf("Total number of bloom filter hits: %"PRId64" (%"PRId64" blocks filtered out)\n",
		   masters.b.hits,(imagefile.blocks-opt_start)-masters.b.hits);
	    printf("Total number of bloom false positives: %"PRId64" (should be low)\n",bloom_false_positives);
	    printf("Prefilter Utilization: %5.2f%% (should be high)\n",prefilter.utilization()*100.0);

	    printf("Bloom filter utilization: %5.2f%% (ideally less than 50%%)\n",
		   masters.b.utilization()*100.0);
	    printf("Time to run: %4.2g seconds\n",runtime);
	}
	if(x) x->pop();
	}

    if(x){
	x->pop();
	x->pop();
	x->close();
	delete x;
    }

    return 0;
}

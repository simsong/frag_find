/**
 * bloom_disk.cpp:
 * Creates a bloom filter for a disk image. Can process AFF file.
 * Defaults to disk sector size, but can be changed. Optionally prints sectors with matching hash.
 * We can then compare two disks by comparing their hashes.
 */


#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIBAFFLIB
#define HAVE_STL
#include "bloom.h"
#include "hash_t.h"
#include "afflib/afflib.h"
#include "afflib/afflib_i.h"
#include "afflib/utils.h"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <err.h>
#include <getopt.h>
#include <ctype.h>

using namespace std;

#include <string>

const char *progname = "bloom_disk";


void usage()
{
    fprintf(stderr,"Usage: %s [options] diskfile bloomfile.bf\n",progname);
    fprintf(stderr,"   Computes a sector-hash for a disk file\n");
    exit(0);
}


int main(int argc,char **argv)
{
    OpenSSL_add_all_digests();
    const EVP_MD *md5 = EVP_get_digestbyname("md5");
    NSRLBloom bf;

    if(access(argv[2],R_OK)==0) err(1,"%s: file exists",argv[2]);
    bf.create(0,128,28,2,string("bloom filter for ")+argv[1]);

    if(!md5) err(1,"Cannot load md5\n");
    AFFILE *af = af_open(argv[1],O_RDONLY,0);
    if(!af) af_err(1,"af_open(%s)",argv[1]);
    aff::seglist segs(af);
    int sector_size = af_get_sectorsize(af);
    u_char *buf = (unsigned char *)calloc(af->image_pagesize,1); 
    for(aff::seglist::const_iterator i=segs.begin();i!=segs.end();i++){
	size_t bytes = af->image_pagesize;
	int64_t pagenum = i->pagenumber();
	if(pagenum<0) continue; // not a data segment
	fprintf(stderr,"Processing Page %"PRId64"\n",pagenum);
	if(af_get_page(af,pagenum,buf,&bytes)!=0){
 	    fprintf(stderr,"  ** can't read page %"PRId64"\n",pagenum);
	    continue;
	}
	for(size_t o=0;o<bytes;o+=sector_size){
	    const unsigned char *sector_buf = buf+o;
	    if(af_is_badsector(af,sector_buf)){
		continue;
	    }

	    /* process the sector */
	    md5_t md5 = md5_generator::hash_buf(sector_buf,sector_size);
	    bf.add(md5.digest);
	}
    }
    bf.write(argv[2]);
    return(0);
}

#else
int main(int argc,char **argv)
{
    fprintf(stderr,"%s currently requires AFFLIB\n",argv[0]);
    exit(1);
}
#endif

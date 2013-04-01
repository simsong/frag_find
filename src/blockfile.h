/** blockfile is a simple abstraction that lets you read a file block-by-block.
 */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __APPLE__
#include <sys/disk.h>			// for DKIOCGETBLOCKSIZE
#endif

#ifdef linux
#include <sys/mount.h>			// for BLKGETSIZE
#endif

class blockfile {
    int    fd;
    struct stat st;
public:
    uint32_t blocksize;
    const char *filename;
    uint64_t filesize;
    uint64_t blocks;			// number of blocks
    blockfile():fd(0),blocksize(512){};
    /* Simple read passthrough */
    ssize_t pread(void *buf,size_t nbyte,off_t len){
	return ::pread(fd,buf,nbyte,len);
    }
    /**
     * open the file and return the fd, or return <0 if error.
     */
    int open(const char *fname,int blocksize){
	this->filename  = fname;
	this->blocksize = blocksize;
	fd = ::open(fname,O_RDONLY | O_BINARY,0666);
	if(fd<0) return fd;
	if(::fstat(fd,&st)<0){
	    ::close(fd);
	    fd = 0;
	    return -1;
	}
	this->filesize = st.st_size;
	this->blocks   = this->filesize / this->blocksize;
	if(this->filesize==0 && (((st.st_mode & S_IFMT)==S_IFBLK) ||
				 ((st.st_mode & S_IFMT)==S_IFCHR))){
	    /* Figure out the size of a block device */
#ifdef __APPLE__
	    int sector_size = 0;
	    uint64_t total_sectors = 0;
	    uint64_t max_read_blocks = 0;
	    if(ioctl(fd,DKIOCGETBLOCKSIZE,&sector_size)){
		sector_size = 512;		// assume 512
	    }
	    if(ioctl(fd,DKIOCGETBLOCKCOUNT,&total_sectors)){
		total_sectors=0;		// seeking not allowed on stdin
	    }
	    if(ioctl(fd,DKIOCGETMAXBLOCKCOUNTREAD,&max_read_blocks)){
		max_read_blocks = 0;	// read all you want
	    }
	    this->blocks   = total_sectors;
	    this->filesize = sector_size * total_sectors;
#endif
#if defined(__FreeBSD__) && defined(DIOCGSECTORSIZE)
	    int sector_size = 0;
	    uint64_t total_sectors = 0;
	    uint64_t max_read_blocks = 0;
	    if(ioctl(fd,DIOCGSECTORSIZE,&sector_size)){
		sector_size = 512;		// can't figure it out; go with the default
	    }
	    off_t inbytes=0;
	    if(ioctl(fd,DIOCGMEDIASIZE,&inbytes)){
		total_sectors = 0;
	    }
	    this->blocks = inbytes / sector_size;
	    this->filesize = inbytes;
#endif
#ifdef linux
#ifdef BLKGETSIZE64
	    uint64_t total_bytes=0;
	    if(ioctl(fd,BLKGETSIZE64,&total_bytes)){
		total_bytes = 0;
	    }
#else
	    int total_bytes=0;
	    if(ioctl(fd,BLKGETSIZE,&total_bytes)){
		total_bytes = 0;
	    }
#endif
	    this->filesize = total_bytes;
	    this->blocks   = total_bytes / 512 /*BLOCK_SIZE*/;
#endif
	}
	return fd;
    }
    ~blockfile(){
	if(fd>0) ::close(fd);
    }

    /**
     * read a block. Return the number of bytes read.
     */

    ssize_t getblock(off_t blocknumber,uint8_t *buf){
	off_t pos = blocknumber * blocksize;
	lseek(fd,pos,SEEK_SET);
	return ::read(fd,buf,blocksize);
    }
};



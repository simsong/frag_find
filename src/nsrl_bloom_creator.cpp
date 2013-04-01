/*
 * nsrl_txt.cpp:
 * Module for working with the nsrl text files
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <fcntl.h>
#include <iostream>
#include <err.h>
#include <regex.h>

using namespace std;

/** Class to describe an entry in the NIST RDS File
 */

regex_t *rds_line = 0;
class RDS_File_Entry {
public:
    string sha1_hex;
    string md5_hex;
    string crc32_hex;
    string FileName;
    off_t  FileSize;
    int    ProductCode;
    string OpSystemCode;
    string SpecialCode;

    RDS_File_Entry(const string &line){
	if(rds_line==0){
	    rds_line = (regex_t *)calloc(1,sizeof(regex_t));
	    if(regcomp(rds_line,
		       "^\"([0-9A-F]{40})\",\"([0-9A-F]{32})\",\"([0-9A-F]{8})\",\"([^\"]+)\","
		       "([0-9]+),([0-9]+),\"([^\"]+)\",\"(^\")*\"",
		       REG_EXTENDED)){
		err(1,"regcomp");
	    }
	}
	regmatch_t pmatch[10];
	memset(pmatch,0,sizeof(pmatch));
	const char *linebuf = line.c_str();
	int res = regexec(rds_line,linebuf,10,pmatch,0);
	if(res==0){
	    sha1_hex = string(linebuf+pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so);
	    md5_hex = string(linebuf+pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so);
	    crc32_hex = string(linebuf+pmatch[3].rm_so,pmatch[3].rm_eo-pmatch[3].rm_so);
	    FileName = string(linebuf+pmatch[4].rm_so,pmatch[4].rm_eo-pmatch[4].rm_so);
	    FileSize = atoi(linebuf+pmatch[5].rm_so);
	    ProductCode = atoi(linebuf+pmatch[6].rm_so);
	    OpSystemCode = string(linebuf+pmatch[7].rm_so,pmatch[7].rm_eo-pmatch[7].rm_so);
	    SpecialCode = string(linebuf+pmatch[8].rm_so,pmatch[8].rm_eo-pmatch[8].rm_so);
	}
	else {
	    cout << "parse failed: " << line << "\n";
	}
    }
    friend ostream & operator << (ostream &os, const RDS_File_Entry &r) ;
};

ostream & operator << (ostream &os, const RDS_File_Entry &r)  {
    os << r.sha1_hex << "/" << r.md5_hex << "/" << r.crc32_hex << " " << r.FileName << "(" << r.FileSize << ") "
       << r.ProductCode << "-" << r.OpSystemCode << "-" << r.SpecialCode ;
    return os;
};
    

/**
 * A class for working with RDSfiles.
 * Reads the file in pages; does a binary search for a given hex SHA1.
 */

class RDSFile {
    //    static const int BUFSIZE = 65536;		// assume no line is bigger than this
    static const int BUFSIZE = 1024;		// assume no line is bigger than this
    char buf[BUFSIZE];			// the buffer
    off_t buf0;				// first byte in buffer
    ssize_t buflen;				// number of bytes in buffer
    int fd;					// pointer to the file
    off_t pos;					// where we are in the file
    off_t next_line;				// position of next line in the file
public:
    RDSFile(string fname):buf0(0),buflen(0),fd(0),pos(0),next_line(0){
	this->fname = fname;
	fd = open(fname.c_str(),O_RDONLY,0);
	if(fd>0){
	    len = lseek(fd,0,SEEK_END);
	}
    }
    ~RDSFile(){
	if (fd>0){
	    close(fd);
	    fd = 0;
	}
    }
    
    void load_buffer(off_t offset){
	if ((offset >= buf0) && (offset < buf0+buflen)){
	    /* Offset is in the buffer. Make sure there is an end-of-line after offset */
	    for(off_t i = offset-buf0;i<buflen;i++){
		if(buf[i]=='\r' || buf[i]=='\n') return; // buffer is loaded!
	    }
	}

	cout << "\nfetch\n";
	buf0 = offset > BUFSIZE/2 ? offset-BUFSIZE/2 : 0;
	buflen = len-buf0 > (off_t)sizeof(buf) ? sizeof(buf) : len-buf0;
	if(lseek(fd,buf0,SEEK_SET)!=buf0) err(1,"lseek");
	if(read(fd,buf,buflen)!=buflen) err(1,"read");
    }

    string getline(off_t offset){	// return the line that contains byte offset
	load_buffer(offset);

	/* Find the beginning of the line */
	off_t line_start = offset-buf0;	// line start in the buffer
	off_t line_end = offset-buf0;	// line end in the buffer
	while(line_start>1 && buf[line_start-1]!='\n' && buf[line_start-1]!='\r') line_start--;

	/* Find the end of the line */
	while(line_end<buflen && buf[line_end]!='\n' && buf[line_end]!='\r') line_end++;

	next_line = line_end+buf0;	// line_end is real coordinate system
	while((next_line-buf0)<buflen && (buf[next_line-buf0]=='\n' || buf[next_line-buf0]=='\r')) next_line++;
	return string(buf+line_start,line_end-line_start);
    }
    string nextline(){
	cout << "next_line=" << next_line << "\n";
	return getline(next_line);
    }
    string fname;
    off_t len;				// size of file
    string sha1_start;
    string sha1_last;
};

int main(int argc,char **argv)
{
    RDSFile r(argv[1]);
    string l = r.getline(atoi(argv[2]));
    cout <<  RDS_File_Entry(l) << "\n";
    for(int i=0;i<10000;i++){
	cout << RDS_File_Entry(r.nextline()) << "\n";
    }
    return 0;
	
}

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <string>

using namespace std;

/**
 * Histogram support functions for sector discrimination.
 */

/*
 * uhistogram:
 * fast implementation of histogram of unigrams.
 */

class char_count {
 public:
    char_count(uint8_t ch,int64_t count):ch(ch),count(count){};
	uint8_t ch;
	int64_t count;
};


class histogram {
    const uint8_t *retained_buf;
    ssize_t retained_bufsize;
 public:
    histogram(){
	memset(counts,0,sizeof(counts));
	retained_buf=0;
	retained_bufsize=0;
    }
    histogram(const uint8_t *buf,size_t bufsize,bool retain){
	memset(counts,0,sizeof(counts));
	if(retain) set(buf,bufsize);
	else update(buf,bufsize);
    }
    
    int64_t operator[] (uint8_t ch) const{
	return counts[ch];
    }

    int64_t counts[256];
    void set(const uint8_t *buf,size_t bufsize){
	update(buf,bufsize);
	retained_buf = buf;
	retained_bufsize = bufsize;
    }

    void update(const uint8_t *buf,size_t bufsize){
	retained_buf = 0;
	retained_bufsize = 0;
	for(size_t i=0;i<bufsize;i++){
	    counts[buf[i]]++;
	}
    }
    void update(std::string str){
	for(size_t i=0;i<str.size();i++){
	    counts[(unsigned)str[i]]++;
	}
    }
    int unique_unigrams() const {
	int total=0;
	for(int i=0;i<256;i++){
	    if(counts[i]) total++;
	}
	return total;
    }
    vector<char_count> get_char_counts() const {
	vector<char_count> ret;
	for(int i=0;i<256;i++){
	    ret.push_back(char_count(i,counts[i]));
	}
	return ret;
    }
    int ngram_count(const std::string &s) const{
	int count = 0;
	if(retained_buf==0) return -1;	/* can't figure it out */
	for(size_t i=0;i<retained_bufsize-s.size();i++){
	    if(s.compare((const char *)retained_buf+i)==0) count++;
	}
	return count;
    }
};

#endif

/*
 * myregex.h:
 * 
 * simple cover for regular expression class.
 * The class allocates and frees the strings 
 */

#ifndef MYREGEX_H
#define MYREGEX_H

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <assert.h>

extern "C" {
#include <regex.h>
}


using namespace std;


class myregex {
 public:
    static const int MYREG_GLOB=0x1000000; /* treat as a shell glob */
    string pat;				   /* our pattern */
    regex_t *ncv;			/* compiled regex */
    myregex(std::string pat_,int flags){
	if(flags & MYREG_GLOB){
	    this->pat = "";
	    for(string::iterator it = pat_.begin();it!=pat_.end();it++){
		if(*it=='?'){ pat+=".";continue;}
		if(*it=='*'){ pat+=".*";continue;}
		this->pat += *it;
	    }
	    flags = flags & ~MYREG_GLOB; /* remove flag */
	} else {
	    this->pat = pat_;
	}
	ncv = 0;
	if(pat.size()==0) return;
	ncv = (regex_t *)calloc(sizeof(*ncv),1);
	if(regcomp(ncv,pat.c_str(),flags | REG_EXTENDED)){
	    std::cerr << "regular expression compile error '" << pat << "'\n";
	    exit(1);
	}

    }
    ~myregex(){
	if(ncv){
	    regfree(ncv);
	    free(ncv);
	    ncv = 0;
	}
    }
    /**
     * perform a search for a single hit. If there is a group and something is found,
     * set *found to be what was found, *offset to be the starting offset, and *len to be
     * the length. Note that this only handles a single group.
     */
     int search(const std::string &line,std::string *found,size_t *offset,size_t *len) const{
	static const int REGMAX=2;
	regmatch_t pmatch[REGMAX];
	if(!ncv) return 0;
	memset(pmatch,0,sizeof(pmatch));
	int r = regexec(ncv,line.c_str(),REGMAX,pmatch,0);
	if(r==REG_NOMATCH) return 0;
	if(r!=0) return 0;		/* some kind of failure */
	/* Make copies of the first group */
	if(pmatch[1].rm_so != pmatch[1].rm_eo){
	    if(found)  *found = line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so);
	    if(offset) *offset = pmatch[1].rm_so;
	    if(len)    *len = pmatch[1].rm_eo-pmatch[1].rm_so;
	}
	return 1;			/* success */
    }
    /** Perform a search with an array of strings. Return 0 if success, return code if fail.*/
    int search(const std::string &line,string *matches,int REGMAX) const {
	regmatch_t *pmatch = (regmatch_t *)calloc(sizeof(regmatch_t),REGMAX+1);
	if(!ncv) return 0;
	int r = regexec(ncv,line.c_str(),REGMAX+1,pmatch,0);
	if(r==0){
	    for(int i=0;i<REGMAX;i++){
		size_t start = pmatch[i+1].rm_so;
		size_t len   = pmatch[i+1].rm_eo-pmatch[i+1].rm_so;
		matches[i] = line.substr(start,len);
	    }
	}
	free(pmatch);
	return r;
    }
    std::string search(const std::string &line) const {
        regmatch_t pmatch[2];
        memset(pmatch,0,sizeof(pmatch));
        if(regexec(ncv,line.c_str(),2,pmatch,0)==0){
            size_t start = pmatch[1].rm_so;
            size_t len   = pmatch[1].rm_eo-pmatch[1].rm_so;
            return line.substr(start,len);
        }
        else {
            return std::string();
        }
    }
};

#endif

#!/usr/bin/python

import sys,os,os.path,time

from subprocess import call,Popen,PIPE
DOMEX_HOME=os.getenv("DOMEX_HOME")
if DOMEX_HOME:
    sys.path.append(DOMEX_HOME + "/src/lib/") # add the library
    sys.path.append(DOMEX_HOME + "/src/fiwalk/python/") # add the library
import fiwalk,hashlib

def test_frag_find(img,args,inodes,test_runs=1):

    if not os.path.exists(img):
        raise RuntimeError,img+" does not exist" 

    fns = []
    for inode in inodes:
        fn = "inode.%s" % str(inode)
        fns += [fn]
        if not os.path.exists(fn):
            cmd = ['icat']
            if args:
                cmd += args
            cmd += [img,str(inode)]
            call(cmd,stdout=open(fn,"wb"))
        else:
            print "%s already exists" % fn

    times = []
    pdfs = " ".join(["inode.%s" % str(inode) for inode in inodes])
    for i in range(0,test_runs):
        t0=time.time()
        call(['/usr/bin/time','./frag_find','-xoutput.xml',img] + fns)
        t1=time.time()
        times.append(t1-t0)             # ignore the first time

    # Now valdate the results
    def process(fi):
        disk_md5 = hashlib.md5(file(fi.filename()).read()).hexdigest()
        image_md5 = hashlib.md5(fi.contents(imagefile=file(img))).hexdigest()
        print "validating ",fi.filename()
        print "XML MD5:   %s" % fi.md5()
        print "Disk MD5:  %s" % disk_md5
        print "Image MD5: %s" % image_md5
        if fi.md5()!=disk_md5 or fi.md5()!=image_md5:
            print "fi.md5()=",fi.md5()
            print "disk_md5=",disk_md5
            print "image_md5=",image_md5
            raise RuntimeError, "*** Validation failed***"

    fiwalk.fiwalk_using_sax(xmlfile=file("output.xml"),callback=process)

    if len(times)>0:
        print ""
        print "========================="
        print "average time of %d trials (ignoring first): %f:" % (len(times),sum(times)/len(times))
        print "========================="

if __name__=="__main__":

    test_frag_find("/corp/drives/nps/nps-2009-domexusers/nps-2009-domexusers.raw",
                   ["-o","63"],
                   ['10348-128-1','10267-128-4','5116-128-3','35416-128-3'])

    exit(0)

    test_frag_find("/corp/drives/nps/nps-2009-casper-rw/ubnist1.casper-rw.gen3.raw",
                   None,
                   [30894,30896,30897])

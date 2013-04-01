/**
 * Java NSRLBloom class to match the C++ class that we have.
 */

import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.FileChannel.MapMode;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class SpeedHash  {
    public static void hashtest(int insize){
	int COUNT=1000000*512/insize+25000;
	String tests[] = {"MD5"};
	System.out.printf("=== Java -- %8d x %6d-byte blocks ===\n\n",COUNT,insize);
	byte[] inbuf = new byte[insize];
	for(int i=0;i<insize;i++){
	    inbuf[i] = 'E';
	}
	
	for(int j=0;j<tests.length;j++){
	    System.out.printf("%7s ...",tests[j]);
	    System.out.printf("\n");
	    AFTimer t = new AFTimer();
	    int r = 0;
	    t.start();
	    for(int i=0;i<COUNT;i++){
		inbuf[0] = (byte)i;
		MD5 md5 = new MD5(inbuf);
	    }
	    t.stop();
	    System.out.printf("time: %8.2f sec; %8.1f Khashes/sec; %8.1f Mb/sec r=%08x\n",
			      t.elapsed_seconds(),
			      ((double)COUNT)/t.elapsed_seconds()/1000.0,
			      ((double)COUNT)*insize/t.elapsed_seconds()/1000000.0,
			      r
			      );
	}
    }
    
    public static void main(String[] args){
	hashtest(512);
	hashtest(4096);
	hashtest(65536);
    }
}
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

public class MD5 {
    MessageDigest md;
    byte[] buf=null;
    MD5(byte[] buf){
	try {
	    md = MessageDigest.getInstance("MD5");
	    md.update(buf);
	    this.buf = md.digest();
	} catch (NoSuchAlgorithmException e) {
	    e.printStackTrace();
	}
    }
    public int hashCode(){
	return (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|(buf[3]);
    }
    public boolean equals(Object b_){
	return compareTo(b_)==0;
    }
    public int compareTo(Object b_){
	MD5 b = (MD5)b_;
	for(int i=0;i<buf.length;i++){
	    if(buf[i] < b.buf[i]) return -1;
	    if(buf[i] > b.buf[i]) return 1;
	}
	return 0;
    }
    public String toString(){
	if(buf==null) return super.toString()+"(buf=null)";
	ByteArrayOutputStream ba = new ByteArrayOutputStream();
	PrintStream p = new PrintStream(ba);
	for(int i=0;i<buf.length;i++){
	    p.printf("%02x",buf[i]);
	}
	return ba.toString();
    }
}



/**
 * Java FragFind class to match the C++ class that we have.
 */

public class AFTimer {
    long t0,t1;
    public void start() {
	t0 = java.lang.System.currentTimeMillis();
    }
    public void stop() {
	t1 = java.lang.System.currentTimeMillis();
    }
    public double elapsed_seconds(){
	return (t1-t0)/1000.0;
    }
}



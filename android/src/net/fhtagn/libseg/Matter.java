package net.fhtagn.libseg;

public class Matter {
    public native String hello();
    
    static {
        System.loadLibrary("libseg");
    }
}

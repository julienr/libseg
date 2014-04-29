package net.fhtagn.libseg;

import android.graphics.Bitmap;
import android.util.Log;

// Java interface to C++ SimpleMatter
public class SimpleMatter {
    private final static String TAG = SimpleMatter.class.getName();
    // The image being matted. Do NOT modify this directly
    public Bitmap image;
    
    // TODO: We need to handle locking somewhat
    
    // The client application should directly modify the bitmaps here and then
    // call updateMatter() to redo the matting 
    public Bitmap fgScribbles;
    public Bitmap bgScribbles;
    
    public Bitmap finalMask;
    
    private long nativeMatter = 0;
    
    public SimpleMatter(Bitmap image) {
        this.image = image;
        this.fgScribbles = Bitmap.createBitmap(image.getWidth(),
                                               image.getHeight(),
                                               Bitmap.Config.ARGB_8888);
        this.bgScribbles = Bitmap.createBitmap(image.getWidth(),
                                               image.getHeight(),
                                               Bitmap.Config.ARGB_8888);
        this.finalMask = Bitmap.createBitmap(image.getWidth(),
                                             image.getHeight(),
                                             Bitmap.Config.ALPHA_8);
        Log.i(TAG, "Hello jni : " + hello());
        nativeMatter = nativeNew(image);
    }
    
    @Override
    protected void finalize() throws Throwable {
        nativeDestroy(nativeMatter);
    }
    
    public synchronized void updateMatter() {
        nativeUpdateMasks(nativeMatter, bgScribbles, fgScribbles);
        nativeGetForegroundMask(nativeMatter, finalMask);
    }
    
    // TODO: JNI methods have to be private ?
    static native String hello();
    
    // Create a new SimpleMatter on the C++ side, returning an identifier
    static native long nativeNew(Bitmap image);
    static native void nativeDestroy(long obj);
    static native void nativeUpdateMasks(long obj, Bitmap bgmask, Bitmap fgmask);
    // Get the foreground mask resulting from the matting
    static native void nativeGetForegroundMask(long obj, Bitmap mask);
   
    static {
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("figtree");
        System.loadLibrary("seg");
    }
}

package net.fhtagn.libseg;

import android.graphics.Bitmap;

// Java interface to C++ SimpleMatter
public class SimpleMatter {
    // The image being matted. Do NOT modify this directly
    public Bitmap image;
    
    // The client application should directly modify the bitmaps here and then
    // call updateMatter() to redo the matting 
    public Bitmap fgScribbles;
    public Bitmap bgScribbles;
    
    public SimpleMatter(Bitmap image) {
        this.image = image;
        this.fgScribbles = Bitmap.createBitmap(image.getWidth(),
                                               image.getHeight(),
                                               Bitmap.Config.ARGB_8888);
        this.bgScribbles = Bitmap.createBitmap(image.getWidth(),
                                               image.getHeight(),
                                               Bitmap.Config.ARGB_8888);
    }
    
    public void updateMatter() {
        
    }
   
    // TODO: Move to native interface
    public native String hello();
   
    static {
        System.loadLibrary("gnustl_shared");
        System.loadLibrary("figtree");
        System.loadLibrary("seg");
    }
}

#include <string.h>
#include <jni.h>

#include <glog/logging.h>
#include <android/bitmap.h>
#include "api.h"

extern "C" JNIEXPORT jstring JNICALL
Java_net_fhtagn_libseg_SimpleMatter_hello(JNIEnv* env, jclass) {
	return env->NewStringUTF("Hello from JNI");
}

extern "C" JNIEXPORT jlong JNICALL
Java_net_fhtagn_libseg_SimpleMatter_nativeNew(
    JNIEnv* env,
    jclass,
    jobject bitmap_image) {
  // TODO RGB => Lab conversion
  // http://www.easyrgb.com/index.php?X=MATH
  AndroidBitmapInfo bm_info;
  AndroidBitmap_getInfo(env, bitmap_image, &bm_info);
  CHECK_EQ(bm_info.format, ANDROID_BITMAP_FORMAT_RGBA_8888)
    << "Need ARGB_8888 format";

  char* pixels;
  AndroidBitmap_lockPixels(env, bitmap_image, (void**)&pixels);

  // TODO: SimpleMatter shouldn't make an internal copy
  const int W = bm_info.width;
  const int H = bm_info.height;
  std::unique_ptr<uint8_t[]> r(new uint8_t[W*H]);
  std::unique_ptr<uint8_t[]> g(new uint8_t[W*H]);
  std::unique_ptr<uint8_t[]> b(new uint8_t[W*H]);

  for (int i = 0; i < H; ++i) {
    for (int j = 0; j < W; ++j) {
      r[i*W + j] = pixels[i*W*4 + j*4];
      g[i*W + j] = pixels[i*W*4 + j*4 + 1];
      b[i*W + j] = pixels[i*W*4 + j*4 + 2];
    }
  }

  SimpleMatter* m = new SimpleMatter(r.get(), g.get(), b.get(), W, H);

  AndroidBitmap_unlockPixels(env, bitmap_image);

  LOG(INFO) << "Created native matter : " << m;
  return (jlong)m;
}


extern "C" JNIEXPORT void JNICALL
Java_net_fhtagn_libseg_SimpleMatter_nativeDestroy(
    JNIEnv* env,
    jclass,
    jlong obj) {
  SimpleMatter* m = (SimpleMatter*)obj;
  LOG(INFO) << "Destroying native matter : " << m;
  delete m;
}

void CheckMaskBitmap(JNIEnv* env, jobject bitmap, int expectedW, int expectedH,
                     AndroidBitmapFormat fmt=ANDROID_BITMAP_FORMAT_RGBA_8888) {
  AndroidBitmapInfo bm_info;
  AndroidBitmap_getInfo(env, bitmap, &bm_info);
  CHECK_EQ(bm_info.format, fmt) << "Wrong format";
  CHECK_EQ(bm_info.width, expectedW);
  CHECK_EQ(bm_info.height, expectedH);
}

extern "C" JNIEXPORT void JNICALL
Java_net_fhtagn_libseg_SimpleMatter_nativeUpdateMasks(
    JNIEnv* env,
    jclass,
    jlong obj,
    jobject bitmap_bgmask,
    jobject bitmap_fgmask) {
  SimpleMatter* m = (SimpleMatter*)obj;
  CheckMaskBitmap(env, bitmap_fgmask, m->GetWidth(), m->GetHeight());
  CheckMaskBitmap(env, bitmap_bgmask, m->GetWidth(), m->GetHeight());

  uint8_t* bg_pix;
  uint8_t* fg_pix;
  AndroidBitmap_lockPixels(env, bitmap_fgmask, (void**)&fg_pix);
  AndroidBitmap_lockPixels(env, bitmap_bgmask, (void**)&bg_pix);

  const int W = m->GetWidth();
  const int H = m->GetHeight();

  // TODO: Avoid copy
  std::unique_ptr<uint8_t[]> fg(new uint8_t[W*H]);
  std::unique_ptr<uint8_t[]> bg(new uint8_t[W*H]);
  for (int i = 0; i < H; ++i) {
    for (int j = 0; j < W; ++j) {
      // Only consider alpha
      fg[i*W + j] = fg_pix[i*W*4 + j*4 + 3];
      bg[i*W + j] = bg_pix[i*W*4 + j*4 + 3];
    }
  }

  m->UpdateMasks(bg.get(), fg.get());

  AndroidBitmap_unlockPixels(env, bitmap_bgmask);
  AndroidBitmap_unlockPixels(env, bitmap_fgmask);
}

extern "C" JNIEXPORT void JNICALL
Java_net_fhtagn_libseg_SimpleMatter_nativeGetForegroundMask(
    JNIEnv* env,
    jclass,
    jlong obj,
    jobject bitmap_mask) {
  SimpleMatter* m = (SimpleMatter*)obj;
  CheckMaskBitmap(env, bitmap_mask, m->GetWidth(), m->GetHeight(),
                  ANDROID_BITMAP_FORMAT_A_8);
  uint8_t* pixels;
  AndroidBitmap_lockPixels(env, bitmap_mask, (void**)&pixels);

  m->GetForegroundMask(pixels);

  AndroidBitmap_unlockPixels(env, bitmap_mask);
}

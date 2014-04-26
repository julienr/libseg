#include <string.h>
#include <jni.h>

jstring
Java_net_fhtagn_libseg_Matter_hello(JNIEnv* env, jobject thiz) {
	return env->NewStringUTF("Hello from JNI");
}

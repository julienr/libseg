LOCAL_PATH := $(call my-dir)

# libseg.a
include $(CLEAR_VARS)
LOCAL_MODULE := libseg
LOCAL_SHARED_LIBRARIES += libfigtree
LOCAL_DEFAULT_CPP_EXTENSION := cc
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include $(LOCAL_PATH)/../../src/third_party/miniglog/
LOCAL_CPPFLAGS := -std=c++11
LOCAL_CFLAGS := -D__GXX_EXPERIMENTAL_CXX0X__
LOCAL_LDLIBS := -llog -ljnigraphics
LOCAL_SRC_FILES := libseg.cc \
									 ../../src/api.cc \
									 ../../src/geodesic.cc \
									 ../../src/kde.cc \
									 ../../src/matting.cc \
									 ../../src/third_party/miniglog/glog/logging.cc
include $(BUILD_SHARED_LIBRARY)

$(call import-module,figtree-android/jni)

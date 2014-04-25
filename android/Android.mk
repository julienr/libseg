LOCAL_PATH := $(call my-dir)

# libseg.a
include $(CLEAR_VARS)
$(call import-module,libfigtree)
LOCAL_MODULE := libseg
LOCAL_SHARED_LIBRARIES += libfigtree
LOCAL_DEFAULT_CPP_EXTENSION := cc
LOCAL_SRC_FILES := ../src/api.cc \
									 ../src/geodesic.cc \
									 ../src/kde.cc \
									 ../src/matting.cc \
									 ../src/third_party/miniglog/glog/logging.cc
include $(BUILD_SHARED_LIBRARY)

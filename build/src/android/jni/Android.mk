LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := net_arcfusion_gemini.cpp
LOCAL_MODULE := net_arcfusion_gemini_lynx
LOCAL_LDLIBS += -llog -landroid -lGLESv2 -lOpenSLES
include $(BUILD_SHARED_LIBRARY)


LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

export SRC_DIR=../../
export DEPS_DIR=../../../dependencies/


LOCAL_C_INCLUDES := ../../ \
					../../core/ \
					../../core/audio \
					../../core/desktop \
					../../core/gldrivers \
					../../contrib

LOCAL_CFLAGS += -DPLATFORM_USE_GLES2=1 -DPLATFORM_INDEX_TYPE=2
LOCAL_SRC_FILES := 	net_arcfusion_gemini.cpp \
					kernel_android.cpp \
					$(SRC_DIR)camera.cpp \
					$(SRC_DIR)configloader.cpp \
					$(SRC_DIR)mathlib.cpp \
					$(SRC_DIR)contrib/stb_image.c \
					$(SRC_DIR)contrib/stb_vorbis.c \
					$(SRC_DIR)core/assets.cpp \
					$(SRC_DIR)core/audio.cpp \
					$(SRC_DIR)core/core_logging.cpp \
					$(SRC_DIR)core/core.cpp \
					$(SRC_DIR)core/filesystem.cpp \
					$(SRC_DIR)core/font.cpp \
					$(SRC_DIR)core/gemgl.cpp \
					$(SRC_DIR)core/gldrivers/opengl_common.cpp \
					$(SRC_DIR)core/gldrivers/opengl_glesv2.cpp \
					$(SRC_DIR)core/image.cpp \
					$(SRC_DIR)core/input.cpp \
					$(SRC_DIR)core/kernel.cpp \
					$(SRC_DIR)core/log.c \
					$(SRC_DIR)core/memory.cpp \
					$(SRC_DIR)core/platform_common.cpp \
					$(SRC_DIR)core/platform.cpp \
					$(SRC_DIR)core/renderer.cpp \
					$(SRC_DIR)core/renderstream.cpp \
					$(SRC_DIR)core/vertexstream.cpp \
					$(SRC_DIR)core/xfile.c \
					$(SRC_DIR)core/xlib.c \
					$(SRC_DIR)core/xnet.c \
					$(SRC_DIR)core/xstr.c \
					$(SRC_DIR)core/xthread.c \
					$(SRC_DIR)core/xtime.c \
					$(SRC_DIR)kernels/helloworld.cpp \
					$(SRC_DIR)kernels/test_universal.cpp \
					$(SRC_DIR)kernels/test_mobile.cpp \
					$(SRC_DIR)keyvalues.cpp \
					$(SRC_DIR)memorystream.cpp \
					$(SRC_DIR)util.cpp


# Add dependencies

# fontstash
LOCAL_C_INCLUDES += $(DEPS_DIR)font-stash/
LOCAL_SRC_FILES += 	$(DEPS_DIR)font-stash/fontstash.c \
					$(DEPS_DIR)font-stash/stb_truetype.c

# glm
LOCAL_C_INCLUDES += $(DEPS_DIR)glm/

# jsoncpp
LOCAL_C_INCLUDES += $(DEPS_DIR)jsoncpp/
LOCAL_SRC_FILES += $(DEPS_DIR)jsoncpp/jsoncpp.cpp
LOCAL_CFLAGS += -DJSON_IS_AMALGAMATION=1 -fexceptions


# murmur3
LOCAL_C_INCLUDES += $(DEPS_DIR)murmur3/
LOCAL_SRC_FILES += $(DEPS_DIR)murmur3/murmur3.c




LOCAL_MODULE = net_arcfusion_gemini_lynx
LOCAL_LDLIBS += -llog -landroid -lGLESv2 -lOpenSLES
include $(BUILD_SHARED_LIBRARY)


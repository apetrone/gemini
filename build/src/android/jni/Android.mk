LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

export SRC_DIR=../../
export DEPS_DIR=../../../dependencies/


LOCAL_C_INCLUDES := ../../ \
					../../core/ \
					../../core/audio \
					../../core/desktop \
					../../core/gldrivers \
					../../contrib \
					../../game

LOCAL_CFLAGS += -DPLATFORM_USE_GLES2=1 -DPLATFORM_INDEX_TYPE=2
LOCAL_SRC_FILES := 	kernel_android.cpp \
					$(SRC_DIR)camera.cpp \
					$(SRC_DIR)configloader.cpp \
					$(SRC_DIR)mathlib.cpp \
					$(SRC_DIR)contrib/stb_image.c \
					$(SRC_DIR)contrib/stb_vorbis.c \
					$(SRC_DIR)core/assets.cpp \
					$(SRC_DIR)core/assets/asset_emitter.cpp \
					$(SRC_DIR)core/assets/asset_material.cpp \
					$(SRC_DIR)core/assets/asset_mesh.cpp \
					$(SRC_DIR)core/assets/asset_shader.cpp \
					$(SRC_DIR)core/assets/asset_spriteconfig.cpp \
					$(SRC_DIR)core/assets/asset_texture.cpp	\
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
					$(SRC_DIR)core/memory.cpp \
					$(SRC_DIR)core/platform_common.cpp \
					$(SRC_DIR)core/platform.cpp \
					$(SRC_DIR)core/renderer.cpp \
					$(SRC_DIR)core/renderstream.cpp \
					$(SRC_DIR)core/vertexstream.cpp \
					$(SRC_DIR)core/xfile.c \
					$(SRC_DIR)debugdraw.cpp \
					$(SRC_DIR)kernels/helloworld.cpp \
					$(SRC_DIR)kernels/test_universal.cpp \
					$(SRC_DIR)kernels/test_mobile.cpp \
					$(SRC_DIR)keyvalues.cpp \
					$(SRC_DIR)memorystream.cpp \
					$(SRC_DIR)util.cpp \
					$(SRC_DIR)render_utilities.cpp \
					$(SRC_DIR)game/componentmanager.cpp \
					$(SRC_DIR)game/componentlibrary.cpp \
					$(SRC_DIR)game/menu.cpp \
					$(SRC_DIR)game/tiledloader.cpp \
					$(SRC_DIR)game/screencontrol.cpp \
					$(SRC_DIR)game/map_event.cpp \
					$(SRC_DIR)game/engine.cpp \
					$(SRC_DIR)game/gamescreen.cpp \
					$(SRC_DIR)game/helpscreen.cpp \
					$(SRC_DIR)game/logoscreen.cpp \
					$(SRC_DIR)game/menuscreen.cpp \
					$(SRC_DIR)game/win_loss_screen.cpp \
					$(SRC_DIR)game/particlesystem.cpp

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

# slim
LOCAL_C_INCLUDES += $(DEPS_DIR)slim/
LOCAL_SRC_FILES += $(DEPS_DIR)slim/slim/xlib.c \
					$(DEPS_DIR)slim/slim/xlog.c \
					$(DEPS_DIR)slim/slim/xnet.c \
					$(DEPS_DIR)slim/slim/xstr.c \
					$(DEPS_DIR)slim/slim/xthread.c \
					$(DEPS_DIR)slim/slim/xtime.c




# if building as JNI lib
LOCAL_MODULE = net_arcfusion_gemini_lynx
LOCAL_SRC_FILES += net_arcfusion_gemini.cpp



LOCAL_LDLIBS += -llog -landroid -lGLESv2 -lOpenSLES

# if building as a native activity
#LOCAL_STATIC_LIBRARIES := android_native_app_glue
#LOCAL_SRC_FILES += native_main.cpp
#LOCAL_LDLIBS += -lEGL
#LOCAL_MODULE := native-activity

# build the shared library
include $(BUILD_SHARED_LIBRARY)


#$(call import-module,android/native_app_glue)


SOURCE_ROOT := /Users/apetrone/gemlin/build
DEPS_ROOT := /Users/apetrone/gemlin/build/dependencies

$(info SOURCE_ROOT = $(SOURCE_ROOT))
$(info DEPS_ROOT = $(DEPS_ROOT))


#
# build core
#
LOCAL_PATH := $(SOURCE_ROOT)/src/core

include $(CLEAR_VARS)

LOCAL_MODULE := core
LOCAL_C_INCLUDES := $(LOCAL_PATH)\
	$(DEPS_ROOT)/glm\
	$(DEPS_ROOT)/murmur3\
	$(SOURCE_ROOT)/src

LOCAL_SRC_FILES := \
	argumentparser.cpp\
	color.cpp\
	datastream.cpp\
	mathlib.cpp\
	mem.cpp\
	str.cpp\
	util.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES += \
	$(DEPS_ROOT)/murmur3/murmur3.c

include $(BUILD_STATIC_LIBRARY)




#
# build platform
#
LOCAL_PATH := $(SOURCE_ROOT)/src/platform

include $(CLEAR_VARS)
LOCAL_MODULE := platform
LOCAL_C_INCLUDES := $(LOCAL_PATH)\
	$(DEPS_ROOT)/glm\
	$(SOURCE_ROOT)/src/core\
	$(SOURCE_ROOT)/src\
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/backend/android\
	$(LOCAL_PATH)/graphics/egl

LOCAL_SRC_FILES := \
	backend/android/android_backend.cpp\
	dylib/posix/posix_dlopen.cpp\
	dylib/posix/posix_dylib.cpp\
	filesystem/android/android_filesystem.cpp\
	filesystem/posix/posix_filesystem_common.cpp\
	graphics/egl/egl_graphics_provider.cpp\
	graphics_provider.cpp\
	input.cpp\
	kernel.cpp\
	platform.cpp\
	time/posix/posix_datetime.cpp\
	time/posix/posix_timer.cpp\
	window.cpp\
	window_provider.cpp\
	window/android/android_window_provider.cpp

# We need to link with the android_native_app_glue static library
# because it will add the include directory to this module.
LOCAL_STATIC_LIBRARIES := core android_native_app_glue

include $(BUILD_STATIC_LIBRARY)


#
# build runtime
#
LOCAL_PATH := $(SOURCE_ROOT)/src/runtime

include $(CLEAR_VARS)
LOCAL_MODULE := runtime
LOCAL_C_INCLUDES := $(LOCAL_PATH)\
	$(DEPS_ROOT)/jsoncpp\
	$(SOURCE_ROOT)/src\
	$(SOURCE_ROOT)/src/runtime\
	$(LOCAL_PATH)

LOCAL_SRC_FILES := \
	configloader.cpp\
	core_logging.cpp\
	core.cpp\
	filesystem_interface.cpp\
	filesystem.cpp\
	logging_interface.cpp

LOCAL_CFLAGS := -DJSON_IS_AMALGAMATION=1
LOCAL_SRC_FILES += \
	$(DEPS_ROOT)/jsoncpp/jsoncpp.cpp

LOCAL_STATIC_LIBRARIES := platform core

include $(BUILD_STATIC_LIBRARY)

#
# build renderer
#
LOCAL_PATH := $(SOURCE_ROOT)/src/renderer

include $(CLEAR_VARS)

LOCAL_MODULE := renderer
LOCAL_C_INCLUDES := $(LOCAL_PATH)\
	$(DEPS_ROOT)/glm\
	$(DEPS_ROOT)/fontstash/src\
	$(DEPS_ROOT)/jsoncpp\
	$(SOURCE_ROOT)/src\
	$(SOURCE_ROOT)/src/contrib\
	$(LOCAL_PATH)\
	$(LOCAL_PATH)/gl\
	$(LOCAL_PATH)/gl/gles2

LOCAL_SRC_FILES := \
	commandbuffer.cpp\
	constantbuffer.cpp\
	debug_draw.cpp\
	font.cpp\
	gl/gemgl.cpp\
	gl/opengl_common.cpp\
	gl/r2_opengl_common.cpp\
	gl/gles2/gles2_device.cpp\
	gl/glcommandserializer.cpp\
	image.cpp\
	material.cpp\
	pipeline.cpp\
	render_utilities.cpp\
	renderer.cpp\
	renderstream.cpp\
	rqueue.cpp\
	shaderconfig.cpp\
	shaderprogram.cpp\
	texture.cpp\
	vertexdescriptor.cpp\
	vertexstream.cpp

LOCAL_CFLAGS := -DJSON_IS_AMALGAMATION=1
LOCAL_SRC_FILES += \
	$(DEPS_ROOT)/jsoncpp/jsoncpp.cpp\
	$(SOURCE_ROOT)/src/contrib/stb_image.c\
	$(SOURCE_ROOT)/src/contrib/stb_truetype.h	

LOCAL_STATIC_LIBRARIES := runtime
include $(BUILD_STATIC_LIBRARY)

#
# build application
#

include $(CLEAR_VARS)
LOCAL_MODULE := gemini_android_application
LOCAL_CFLAGS := -DJSON_IS_AMALGAMATION=1
LOCAL_C_INCLUDES := \
	$(DEPS_ROOT)/glm\
	$(SOURCE_ROOT)/src\
	$(SOURCE_ROOT)/tests


#LOCAL_SRC_FILES := $(SOURCE_ROOT)/tests/src/test_app.cpp
LOCAL_SRC_FILES := $(SOURCE_ROOT)/tests/src/test_render.cpp
LOCAL_STATIC_LIBRARIES := renderer runtime platform core android_native_app_glue
LOCAL_LDLIBS := -ldl -lEGL -lGLESv2 -landroid -llog
include $(BUILD_SHARED_LIBRARY)


$(call import-module,android/native_app_glue)
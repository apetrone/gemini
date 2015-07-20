# all x86 mips armeabi armeabi-v7a
APP_ABI := armeabi-v7a
APP_PLATFORM := android-21

APP_CPPFLAGS := \
	-std=c++11\
	-fexceptions\
	-DJSON_IS_AMALGAMATION=1\
	-DPLATFORM_GLES2_SUPPORT=1\
	-DPLATFORM_EGL_SUPPORT=1\
	-D__STDINT_LIMITS=1


NDK_TOOLCHAIN_VERSION := 4.9
#NDK_TOOLCHAIN_VERSION := clang

# To select the static STLport implementation provided with this NDK. Value APP_STL values are the following:
# system -> Use the default minimal C++ runtime library.
# stlport_static -> Use STLport built as a static library.
# stlport_shared -> Use STLport built as a shared library.
# gnustl_static -> Use GNU libstdc++ as a static library.
APP_STL := gnustl_static

# set to debug or release
APP_OPTIM := debug
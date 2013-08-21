# all x86 mips armeabi armeabi-v7a
APP_ABI := armeabi-v7a
APP_PLATFORM := android-9

# To select the static STLport implementation provided with this NDK. Value APP_STL values are the following:
# system -> Use the default minimal C++ runtime library.
# stlport_static -> Use STLport built as a static library.
# stlport_shared -> Use STLport built as a shared library.
# gnustl_static -> Use GNU libstdc++ as a static library.
APP_STL := gnustl_static

# set to debug or release
APP_OPTIM := debug

# -fno-rtti is set by default on Android; let's enable RTTI here.
APP_CPPFLAGS := -frtti
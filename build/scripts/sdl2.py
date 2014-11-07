import os
import logging

from pegasus.models import Product, ProductType

SDL_SUBSYSTEMS = """Atomic Audio Video Render Events
	Joystick Haptic Power Threads Timers
	File Loadso CPUinfo Filesystem""".replace("\n", " ").split(" ")

def arguments(p):

	p.add_argument("--disk-audio", 				dest="disk_audio", 					default=True, 		help="Support the disk writer audio driver")
	p.add_argument("--dummy-audio", 			dest="dummy_audio", 				default=True, 		help="Support the dummy audio driver")
	p.add_argument("--video-dummy", 			dest="video_dummy", 				default=True, 		help="Use dummy video driver")
	p.add_argument("--video-opengl", 			dest="video_opengl", 				default=True, 		help="Support OpenGL")
	p.add_argument("--video-opengles", 			dest="video-opengles", 				default=True, 		help="Support OpenGL ES")
	
	p.add_argument("--pthreads", 				dest="pthreads", 					default=True, 		help="Use POSIX threads for multi-threading")
	p.add_argument("--pthreads-sem", 			dest="pthreads_sem", 				default=True, 		help="Use pthread semaphores")
	p.add_argument("--dlopen",					dest="dlopen",						default=True,		help="Use dlopen for shared object loading")

	# sound APIs
	p.add_argument("--audio-oss",				dest="audio_oss",					default=True,		help="Support the OSS audio API")
	p.add_argument("--audio-alsa",				dest="audio_alsa",					default=True,		help="Support the ALSA audio API")
	p.add_argument("--audio-alsa-shared",		dest="audio_alsa_shared",			default=True,		help="Dynamically load the ALSA audio support (mutually exclusive with --sound-alsa)")
	p.add_argument("--audio-esd",				dest="audio_esd",					default=True,		help="Support the Enlightened Sound Daemon")
	p.add_argument("--audio-esd-shared",		dest="audio_esd_shared",			default=True,		help="Dynamically load the ESD audio support")
	p.add_argument("--audio-pulseaudio",		dest="audio_pulseaudio",			default=True,		help="Use PulseAudio")
	p.add_argument("--audio-pulseaudio-shared", dest="audio_pulseaudio_shared",		default=True,		help="Dynamically load PulseAudio support")
	p.add_argument("--audio-arts",				dest="audio_arts",					default=True,		help="Support the Analog Real Time Synthesizer")
	p.add_argument("--audio-arts-shared",		dest="audio_arts_shared",			default=True,		help="Dynamically load ARTS audio support")
	p.add_argument("--audio-nas",				dest="audio_nas",					default=True,		help="Support the NAS audio API")
	p.add_argument("--audio-nas-shared",		dest="audio_nas_shared",			default=True,		help="Dynamically load NAS audio API")
	p.add_argument("--audio-sndio",				dest="audio_sndio",					default=True,		help="Support the sndio audio API")

	p.add_argument("--rpath",					dest="rpath",						default=True,		help="Use an rpath when linking SDL")
	p.add_argument("--clock-gettime", 			dest="clock_gettime", 				default=False, 		help="Use clock_gettime() instead of gettimeofday()")
	p.add_argument("--input-tslib", 			dest="input_tslib", 				default=False, 		help="Use the Touchscreen library for input")

	p.add_argument("--video-x11", 				dest="video_x11", 					default=False, 		help="Use X11 video driver")
	p.add_argument("--video-wayland", 			dest="video_wayland", 				default=False, 		help="Use Wayland video driver")
	p.add_argument("--video-mir", 				dest="video_mir", 					default=False, 		help="Use Mir video driver")

	p.add_argument("--with-libc",				dest="with_libc",					default=True,		help="Compile with libc functionality")

	p.add_argument("--video-cocoa",				dest="video_cocoa",					default=True,		help="Use Cocoa video driver")
	p.add_argument("--directx",					dest="directx",						default=True,		help="Use DirectX for Windows audio/video")
	p.add_argument("--render_d3d",				dest="render_d3d",					default=True,		help="Enable the Direct3D render driver")

	p.add_argument("--x11-shared",				dest="x11_shared",					default=True,		help="Dynamically load X11 support")

	# add options for subsystems
	global SDL_SUBSYSTEMS
	for subsystem in SDL_SUBSYSTEMS:
		name = "--with-%s" % subsystem.lstrip().lower()
		dest = name[2:].replace("-", "_")
		help = "Enable the %s subsystem" % subsystem
		p.add_argument(name, dest=dest, help=help, default=1, type=int)

def check_pulseaudio(arguments, product, target_platform, vars):
	logging.info("TODO: check_library_exists: libpulse-simple")
	if False: # library found
		#vars["HAVE_PULSEAUDIO"] = True

		#vars["SDL_AUDIO_DRIVER_PULSEAUDIO"] = 1

		pass

def check_esd(arguments, product, target_platform, vars):
	logging.info("TODO: check ESD")

def check_arts(arguments, product, target_platform, vars):
	logging.info("TODO: check ARTS")

def check_nas(arguments, product, target_platform, vars):
	logging.info("TODO: check NAS")

def check_sndio(arguments, product, target_platform, vars):
	logging.info("TODO: check_sndio")

def check_fusionsound(arguments, product, target_platform, vars):
	logging.info("TODO: check fusionsound")


def check_X11(arguments, product, target_platform, vars):
	x11_libs = [
		"X11", "Xext", "Xcursor", "Xinerama", "Xi", "Xrandr", "Xrender",
		"Xss", "Xxf86vm"
	]

	libs = {}
	for library in x11_libs:
		libs[library] = target_platform.find_library(library)
		library_path = libs[library].path if libs[library] else ""
		logging.info("find_library %s...%s (%s)" % (library, "FOUND" if libs[library] else "NOT FOUND", library_path))

	x_includedir = target_platform.find_include_path("X11/Xlib.h")
	product.includes += [x_includedir]

	vars["HAVE_XCURSOR_H"] = target_platform.check_include_exists("X11/Xcursor/Xcursor.h")
	vars["HAVE_XINERAMA_H"] = target_platform.check_include_exists("X11/extensions/Xinerama.h")
	vars["HAVE_XINPUT_H"] = target_platform.check_include_exists("X11/extensions/XInput2.h")
	vars["HAVE_XRANDR_H"] = target_platform.check_include_exists("X11/extensions/Xrandr.h")
	vars["HAVE_XRENDER_H"] = target_platform.check_include_exists("X11/extensions/Xrender.h")
	vars["HAVE_XSS_H"] = target_platform.check_include_exists("X11/extensions/scrnsaver.h")
	vars["HAVE_XSHAPE_H"] = target_platform.check_include_exists("X11/extensions/shape.h")
	vars["HAVE_XF86VM_H"] = target_platform.check_include_exists(["X11/Xlib.h", "X11/extensions/xf86vmode.h"])
	vars["HAVE_XEXT_H"] = target_platform.check_include_exists(["X11/Xlib.h", "X11/Xproto.h", "X11/extensions/Xext.h"])


	if libs["X11"]:
		if not vars["HAVE_XEXT_H"]:
			raise Exception("Missing Xext.h, maybe you need to install the libxext-dev package?")

	vars["HAVE_VIDEO_X11"] = True
	vars["HAVE_SDL_VIDEO"] = True

	product.sources += ["src/video/x11/*.c"]
	vars["SDL_VIDEO_DRIVER_X11"] = True

	if target_platform.get() in ["iphoneos", "iphonesimulator", "macosx"]:
		arguments.x11_shared = False

	have_shmat = target_platform.check_function_exists("shmat")
	if not have_shmat:
		have_shmat = target_platform.check_function_exists("shmat", linkflags=["-lipc"])
		if have_shmat:
			logging.info("have shmat")
			product.links += ["ipc"]

	if arguments.x11_shared and libs["X11"]:
		if not vars["HAVE_DLOPEN"]:
			logging.warn("You must have SDL_LoadObject() support for dynamic X11 loading")
			vars["HAVE_X11_SHARED"] = False
		else:
			vars["HAVE_X11_SHARED"] = True

		if vars["HAVE_X11_SHARED"]:
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC"] = libs["X11"].soname
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT"] = libs["Xext"].soname

		else:
			# fall back to static links
			product.links += [
				"X11",
				"Xext"
			]
		

	vars["SDL_VIDEO_DRIVER_X11_CONST_PARAM_XEXTADDDISPLAY"] = target_platform.check_source_compiles(
	"""
		#include <X11/Xlib.h>
		#include <X11/Xproto.h>
		#include <X11/extensions/Xext.h>
		#include <X11/extensions/extutil.h>
		extern XExtDisplayInfo* XextAddDisplay(XExtensionInfo* a,Display* b,_Xconst char* c,XExtensionHooks* d,int e,XPointer f);
		int main(int argc, char **argv) {}"
	""")

	vars["SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS"] = target_platform.check_source_compiles(
	"""
		#include <X11/Xlib.h>
		int main(int argc, char **argv) {
		Display *display;
		XEvent event;
		XGenericEventCookie *cookie = &event.xcookie;
		XNextEvent(display, &event);
		XGetEventData(display, cookie);
		XFreeEventData(display, cookie); }
	"""
	)

	vars["SDL_VIDEO_DRIVER_X11_CONST_PARAM_XDATA32"] = target_platform.check_source_compiles(
	"""
		#include <X11/Xlibint.h>
		extern int _XData32(Display *dpy,register _Xconst long *data,unsigned len);
		int main(int argc, char **argv) {}
	"""
	)

	vars["SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM"] = target_platform.check_function_exists("XkbKeycodeToKeysym")

	if vars["HAVE_XCURSOR_H"]:
		vars["HAVE_VIDEO_X11_XCURSOR"] = True
		if vars["HAVE_X11_SHARED"] and libs["Xcursor"]:
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC_XCURSOR"] = libs["Xcursor"].soname
		else:
			product.links += ["Xcursor"]
		vars["SDL_VIDEO_DRIVER_X11_XCURSOR"] = True

	if vars["HAVE_XINERAMA_H"]:
		if vars["HAVE_X11_SHARED"] and libs["Xinerama"]:
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC_XINERAMA"] = libs["Xinerama"].soname
		else:
			product.links += ["Xinerama"]
		vars["SDL_VIDEO_DRIVER_X11_XINERAMA"] = True

	if vars["HAVE_XINPUT_H"]:
		if vars["HAVE_X11_SHARED"] and libs["Xi"]:
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT2"] = libs["Xi"].soname
		else:
			product.links += ["Xi"]
		vars["SDL_VIDEO_DRIVER_X11_XINPUT2"] = True

		logging.info("TODO: check for Xinput2 multitouch")


	if vars["HAVE_XRANDR_H"]:
		if vars["HAVE_X11_SHARED"] and libs["Xrandr"]:
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR"] = libs["Xrandr"].soname
		else:
			product.links += ["Xrandr"]
		vars["SDL_VIDEO_DRIVER_X11_XRANDR"] = True

	if vars["HAVE_XSS_H"]:
		if vars["HAVE_X11_SHARED"] and libs["Xss"]:
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS"] = libs["Xss"].soname
		else:
			product.links += ["Xss"]
		vars["SDL_VIDEO_DRIVER_X11_XSS"] = True

	if vars["HAVE_XSHAPE_H"]:
		vars["SDL_VIDEO_DRIVER_X11_XSHAPE"] = True

	if vars["HAVE_XF86VM_H"]:
		if vars["HAVE_X11_SHARED"] and libs["Xxf86vm"]:
			vars["SDL_VIDEO_DRIVER_X11_DYNAMIC_XVIDMODE"] = libs["Xxf86vm"].soname
		else:
			product.links += ["Xxf86vm"]
		vars["SDL_VIDEO_DRIVER_X11_XVIDMODE"] = True

def check_video_mir(arguments, product, target_platform, vars):
	logging.info("TODO: check Mir")

def check_video_directfb(arguments, product, target_platform, vars):
	logging.info("TODO: check DirectFB")

def check_opengl_x11(arguments, product, target_platform, vars):
	vars["HAVE_VIDEO_OPENGL"] = target_platform.check_source_compiles(
	"""
	#include <GL/gl.h>
	#include <GL/glx.h>
	int main(int argc, char** argv) {}
	""")

	if vars["HAVE_VIDEO_OPENGL"]:
		vars["SDL_VIDEO_OPENGL"] = True
		vars["SDL_VIDEO_OPENGL_GLX"] = True
		vars["SDL_VIDEO_RENDER_OGL"] = True
		product.links += ["GL"]

	logging.info("checking for OpenGL and GLX...%s" % "Found" if vars["HAVE_VIDEO_OPENGL"] else "Not found")

def check_opengles_x11(arguments, product, target_platform, vars):
	vars["HAVE_VIDEO_EGL"] = target_platform.check_source_compiles(
		"""
		#include <EGL/egl.h>
		int main (int argc, char** argv) {}
		"""
	)

	if vars["HAVE_VIDEO_EGL"]:
		vars["SDL_VIDEO_OPENGL_EGL"] = True
	logging.info("checking for EGL...%s" % ("Found" if vars["HAVE_VIDEO_EGL"] else "Not found"))		

	vars["SDL_VIDEO_OPENGL_ES" ] = target_platform.check_source_compiles(
		"""
		#include <GLES/gl.h>
		#include <GLES/glext.h>
		int main (int argc, char** argv) {}
		"""
	)
	if vars["SDL_VIDEO_OPENGL_ES" ]:
		vars["SDL_VIDEO_RENDER_OGL_ES"] = True
	logging.info("checking for OpenGL ES 1.0...%s" % ("Found" if vars["SDL_VIDEO_OPENGL_ES"] else "Not found"))

	vars["SDL_VIDEO_OPENGL_ES2"] = target_platform.check_source_compiles(
		"""
		#include <GLES2/gl2.h>
		#include <GLES2/gl2ext.h>
		int main (int argc, char** argv) {}
		"""
	)
	if vars["SDL_VIDEO_OPENGL_ES2"]:
		vars["SDL_VIDEO_RENDER_OGL_ES2"] = True
	logging.info("checking for OpenGL ES 2.0...%s" % ("Found" if vars["SDL_VIDEO_OPENGL_ES2"] else "Not found"))


def check_wayland(arguments, product, target_platform, vars):
	logging.info("TODO: check Wayland")



def check_dlopen(arguments, product, target_platform, vars):
	have_dlopen = target_platform.check_function_exists("dlopen")
	

	dllib = None

	if not have_dlopen:
		libraries = ["dl", "tdl"]
		for lib in libraries:
			if target_platform.check_function_exists("dlopen", includes=["dlfcn.h"], linkflags=["-l%s" % lib]):
				logging.info("check_dlopen: found library, \"%s\"" % lib)
				have_dlopen = True
				dllib = lib
				break

	if have_dlopen:

		logging.info("dlopen is available")

		product.links += [dllib]

		have_dlopen = target_platform.check_source_compiles("""
			#include <dlfcn.h>
				int main(int argc, char **argv) {
				void *handle = dlopen(\"\", RTLD_NOW);
				const char *loaderror = (char *) dlerror();
			}""")

		if have_dlopen:
			vars["SDL_LOADSO_DLOPEN"] = have_dlopen
			product.sources += ["src/loadso/dlopen/*.c"]
	else:
		logging.warn("dlopen not available")

	vars["HAVE_DLOPEN"] = have_dlopen

	return have_dlopen

def check_pthread(arguments, product, target_platform, vars):
	# check for pthreads
	pthread_cflags = []
	pthread_linkflags = []
	if target_platform.matches("linux"):
		pthread_cflags += ["-D_REENTRANT"]
		pthread_linkflags += ["-lpthread"]
	elif target_platform.matches("macosx"):
		pthread_cflags += ["-D_REENTRANT", "-D_THREAD_SAFE"]

	vars["SDL_THREAD_PTHREAD"] = target_platform.check_source_runs("""
		#include <pthread.h>
		int main(int argc, char** argv)
		{
			pthread_attr_t type;
			pthread_attr_init(&type);
			return 0;
		}""", cflags=pthread_cflags, linkflags=pthread_linkflags)

	if vars["SDL_THREAD_PTHREAD"]:
		# add pthread cflags to cflags
		# add pthread linkflags to linkflags
		product.cflags.extend(pthread_cflags)
		if pthread_linkflags:
			product.links += ["pthread"]

		vars["SDL_THREAD_PTHREAD_RECURSIVE_MUTEX"] = target_platform.check_source_compiles("""
		#include <pthread.h>
		int main(int argc, char **argv)
		{
			pthread_mutexattr_t attr;
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			return 0;
		}""")

		if vars["SDL_THREAD_PTHREAD_RECURSIVE_MUTEX"]:
			vars["SDL_THREAD_PTHREAD_RECURSIVE_MUTEX_NP"] = target_platform.check_source_compiles("""
				#include <pthread.h>
				int main(int argc, char **argv)
				{
					pthread_mutexattr_t attr;
					pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
					return 0;
				}""")

		have_pthreads_timedwait = False
		have_pthreads_sem = False
		if arguments.pthreads_sem:
			have_pthreads_sem = target_platform.check_source_compiles("""
				#include <pthread.h>
				#include <semaphore.h>
				int main(int argc, char **argv) { return 0; }""")

			if have_pthreads_sem:
				vars["HAVE_SEM_TIMEDWAIT"] = target_platform.check_source_compiles("""
					#include <pthread.h>
					#include <semaphore.h>
					int main(int argc, char **argv)
					{
						sem_timedwait(NULL, NULL);
						return 0;
					}""")

				product.sources += ["src/thread/pthread/SDL_syssem.c"]
			else:
				product.sources += ["src/thread/generic/SDL_syssem.c"]

		vars["HAVE_PTHREAD_SPINLOCK"] = target_platform.check_source_compiles("""
			#include <pthread.h>
			int main(int argc, char** argv) {
				pthread_spin_trylock(NULL);
				return 0;
			}""")

		vars["HAVE_PTHREAD_SETNAME_NP"] = target_platform.check_source_compiles(
			"""
			#include <pthread.h>
			int main()
			{
				pthread_t test = 0;
				pthread_setname_np(test, "test");
				return 0;
			}
			""", includes=["pthread.h"], linkflags=pthread_linkflags)

		vars["HAVE_PTHREAD_SET_NAME_NP"] = target_platform.check_function_exists("pthread_set_name_np", includes=["pthread.h"], linkflags=pthread_linkflags)

		vars["HAVE_PTHREAD_NP_H"] = target_platform.check_source_compiles("""
          #include <pthread.h>
          #include <pthread_np.h>
          int main(int argc, char** argv) { return 0; }""")


		product.sources += [
			"src/thread/pthread/SDL_systhread.c",
			"src/thread/pthread/SDL_sysmutex.c",
			"src/thread/pthread/SDL_syscond.c",
			"src/thread/pthread/SDL_systls.c"
		]

		if have_pthreads_sem:
			pass

def check_directx(vars, arguments, product, target_platform):
	header_define_map = {
		"d3d9.h" : "D3D",
		"d3d11_1.h" : "D3D11",
		"ddraw.h" : "DDRAW",
		"dsound.h" : "DSOUND",
		"dinput.h" : "DINPUT",
		"xaudio2.h" : "XAUDIO2",
		"xinput.h" : "XINPUT",
		"dxgi.h" : "DXGI"
	}

	have_directx_headers = False
	for header, name in header_define_map.iteritems():
		key = "HAVE_%s_H" % name
		vars[key] = target_platform.check_include_exists(header)
		logging.info("check %s (%s) -> %s" % (header, key, vars[key]))
		if vars[key]:
			have_directx_headers = True

	# Since Microsoft changed the way they distribute DirectX after June2010,
	# (most headers are now included with the Windows Platform SDK)
	# We also have to detect if the $(DXSDK) is in the environment.
	# This will indicate that the DXSDK is installed -- which is required by SDL2
	# because it doesn't seem to support these headers in the Platform SDK.



	if have_directx_headers and "DXSDK_DIR" in os.environ:
		dxsdk_root = os.environ["DXSDK_DIR"]
		logging.info("Detected DirectX SDK in: %s" % dxsdk_root)
		
		product.includes += [
			"$(DXSDK_DIR)/Include"
		]

		#product.libdirs += [
		#	"%s/Lib/${ARCHITECTURE}" % dxsdk_root
		#]
		


def get_config_variables(arguments, product, target_platform):
	vars = {}
	global SDL_SUBSYSTEMS

	# check for various subsystems
	for subsystem in SDL_SUBSYSTEMS:
		name = "with-%s" % subsystem.lstrip().lower()
		attrib_name = name.replace("-", "_")
		if hasattr(arguments, attrib_name):
			value = getattr(arguments, attrib_name, 1)
			if value == 0:
				name = subsystem.lstrip().upper()
				vars["SDL_%s_DISABLED" % name] = True

	product.cflags += ["-DUSING_GENERATED_CONFIG_H"]

	#for system in subsystems:
	#	varname = "%s_OPT" % system.upper()


	#
	# check compiler and flags
	#

	# check compatibility for -Wall
	if (target_platform.check_source_compiles(
		"""
		int main(int argc, char **argv)
		{
			return 0;
		}
		""",
		cflags=["-Wall"])):
		product.cflags += ["-Wall"]


	# only required for haiku builds
	#product.cflags += ["-Wno-multichar"]

	vars["HAVE_GCC_ATOMICS"] = target_platform.check_source_compiles(
		"""int main(int argc, char **argv) {
        int a;
        void *x, *y, *z;
        __sync_lock_test_and_set(&a, 4);
        __sync_lock_test_and_set(&x, y);
        __sync_fetch_and_add(&a, 1);
        __sync_bool_compare_and_swap(&a, 5, 10);
        __sync_bool_compare_and_swap(&x, y, z); }""")

	if not vars["HAVE_GCC_ATOMICS"]:
		vars["HAVE_GCC_SYNC_LOCK_TEST_AND_SET"] = target_platform.check_source_compiles(
			"""check_c_source_compiles("int main(int argc, char **argv) {
          int a;
          __sync_lock_test_and_set(&a, 1);
          __sync_lock_release(&a); }""")


	if target_platform.check_source_compiles("int x = 0; int main(int argc, char **argv) {}", cflags=["-mpreferred-stack-boundary=2"]):
		product.cflags += ["-mpreferred-stack-boundary=2"]

	visibility_source = """
		#if !defined(__GNUC__) || __GNUC__ < 4
		#error SDL only uses visibility attributes in GCC 4 or newer
		#endif
		int main(int argc, char **argv) {}"""

	if target_platform.check_source_compiles(visibility_source, cflags=["-fvisibility=hidden", "-Werror"]):
		product.cflags += ["-fvisibility=hidden"]

	#
	# check for libc
	#
	libc_platforms = [
		"linux",
		"macosx",
		"iphoneos",
		"iphonesimulator"
	]

	USEFUL_HEADERS = [
		"sys/types.h", "stdio.h", "stdlib.h", "stddef.h", "stdarg.h", 
		"malloc.h", "memory.h", "string.h", "strings.h", "inttypes.h", 
		"stdint.h", "ctype.h", "math.h", "iconv.h", "signal.h",
		"unistd.h", "time.h", "setjmp.h"
	]

	USEFUL_SYMBOLS = [
		"getenv", "setenv", "putenv", "unsetenv", "bcopy",
		"malloc", "calloc", "realloc", "free", "qsort", "abs", "memset",
		"memcpy", "memmove", "memcmp", "strlen", "strlcpy", "strlcat",
		"_strrev", "_strupr", "strdup", "_uitoa", "_i64toa", "_ui64toa",
		"_strlwr", "strchr", "strrchr", "strstr", "itoa", "_ltoa",
		"strtoull", "strcasecmp", "strncasecmp", "vsscanf", "vsnprintf", 
		"fseeko", "fseeko64", "sigaction", "setjmp", "nanosleep", "sysconf",
		"sysctlbyname",	"_ultoa", "strtol", "strtoul", "strtoll", "strtod", 
		"atoi",	"atof", "strcmp", "strncmp", "_stricmp", "_strnicmp",
		"sscanf", "atan", "atan2", "acos", "asin", "ceil", "copysign", "cos", 
		"cosf", "fabs", "floor", "log", "pow", "scalbn", "sin", "sinf",
		"sqrt"
	]

	STDC_HEADERS = [
		"dlfcn.h", "stdint.h", "stddef.h", "inttypes.h", "stdlib.h", 
		"strings.h", "string.h", "float.h"
	]

	available_headers = []

	for header in USEFUL_HEADERS:
		varname = header.replace(".", "_").replace("/", "_")
		varname = "HAVE_%s" % varname.upper()
		vars[varname] = target_platform.check_include_exists(header)
		if vars[varname]:
			available_headers.append(header)

	if target_platform.get() in libc_platforms:
		vars["HAVE_LIBC"] = True

		# if not cygwin and not mingw
		vars["HAVE_ALLOCA"] = True

		product.cflags += ["-D_USE_MATH_DEFINES"]

	else:
		logging.info("TODO: check sizeof(size_t); export to SIZEOF_SIZE_T")

		logging.info("TODO: check_library_exists: m pow")

	vars["HAVE_M_PI"] = target_platform.check_function_exists("M_PI", includes=["math.h"])
	vars["STDC_HEADERS"] = target_platform.check_include_exists(STDC_HEADERS)
	vars["HAVE_SA_SIGACTION"] = target_platform.check_source_compiles(
		"""
		#include <signal.h>
		int main(){
			struct sigaction sa;
			sa.sa_sigaction;
			return 0;
		}"""
	)
		

	vars["HAVE_ICONV"] = target_platform.check_source_compiles(
		"""
		#include <iconv.h>
		int main() {
			iconv_open("from", "to");
			return 0;
		}
		""",
		includes=["iconv.h"]
	)

	vars["HAVE_MPROTECT"] = target_platform.check_source_compiles("""
		#include <sys/types.h>
		#include <sys/mman.h>
		int main() { return 0; }
		""")

	for symbol in USEFUL_SYMBOLS:
		varname = symbol.replace(".", "_").replace("/", "_")
		varname = "HAVE_%s" % varname.upper()
		vars[varname] = target_platform.check_function_exists(symbol, includes=available_headers, cpp=True)

	have_dlopen = False
	if arguments.with_loadso:
		if target_platform.get() in ["macosx", "linux"]:
			have_dlopen = check_dlopen(arguments, product, target_platform, vars)

	if arguments.with_joystick:
		product.sources += ["src/joystick/*.c"]		

	if arguments.with_haptic:
		product.sources += ["src/haptic/*.c"]

	if arguments.with_power:
		product.sources += ["src/power/*.c"]

	if arguments.with_audio:
		if arguments.disk_audio:
			product.sources += ["src/audio/disk/*.c"]
			vars["SDL_AUDIO_DRIVER_DISK"] = True
		if arguments.dummy_audio:
			product.sources += ["src/audio/dummy/*.c"]
			vars["SDL_AUDIO_DRIVER_DUMMY"] = True

		if target_platform.matches("linux"):
			# check oss
			if arguments.audio_oss:
				headers = ["sys/soundcard.h", "soundcard.h"]
				found_oss = False
				for found_header in headers:
					found_oss = target_platform.check_source_compiles(
						"""
						#include <%s>
						int main() { int arg = SNDCTL_DSP_SETFRAGMENT; }
						""" % found_header
					)
					if found_oss:
						break

				if found_oss:
					logging.info("oss support found via \"%s\"" % found_header)
					product.sources += ["src/audio/dsp/*.c"]
					if found_oss == "soundcard.h":
						vars["SDL_AUDIO_DRIVER_OSS_SOUNDCARD_H"] = True
					vars["SDL_AUDIO_DRIVER_OSS"] = True

					logging.info("TODO: if netbsd or openbsd; add link: ossaudio")
					vars["HAVE_SDL_AUDIO"] = True

			# check alsa
			if arguments.audio_alsa:
				has_asoundlib = target_platform.check_include_exists("alsa/asoundlib.h")
				if has_asoundlib:
					logging.info("TODO: check_library_exists: asound snd_pcm_open")

					product.sources += ["src/audio/alsa/*.c"]

					vars["SDL_AUDIO_DRIVER_ALSA"] = True
					if arguments.audio_alsa_shared:
						if not have_dlopen:
							raise Exception("You must have SDL_LoadObject() support for dynamic ALSA loading. dlopen not found!")
						else:
							logging.info("TODO: find library and soname: asound")
					else:
						product.links += ["asound"]

			# check audio
			check_pulseaudio(arguments, product, target_platform, vars)
			check_esd(arguments, product, target_platform, vars)
			check_arts(arguments, product, target_platform, vars)
			check_nas(arguments, product, target_platform, vars)
			check_sndio(arguments, product, target_platform, vars)
			check_fusionsound(arguments, product, target_platform, vars)

	if arguments.with_video:
		if arguments.video_dummy:
			vars["SDL_VIDEO_DRIVER_DUMMY"] = True
			product.sources += ["src/video/dummy/*.c"]

		if target_platform.matches("linux"):
			check_X11(arguments, product, target_platform, vars)
			check_video_mir(arguments, product, target_platform, vars)
			check_video_directfb(arguments, product, target_platform, vars)
			check_opengl_x11(arguments, product, target_platform, vars)
			check_opengles_x11(arguments, product, target_platform, vars)
			check_wayland(arguments, product, target_platform, vars)

	if target_platform.matches("linux"):
		product.sources += ["src/core/linux/*.c"]

		vars["SDL_INPUT_LINUXEV"] = target_platform.check_source_compiles("""
			#include <linux/input.h>
			#ifndef EVIOCGNAME
			#error EVIOCGNAME() ioctl not available
			#endif
			int main(int argc, char** argv) {}""")

		vars["SDL_INPUT_LINUXKD"] = target_platform.check_source_compiles("""
			#include <linux/kd.h>
			#include <linux/keyboard.h>

			int main(int argc, char **argv) 
			{
				struct kbentry kbe;
				kbe.kb_table = KG_CTRL;
				ioctl(0, KDGKBENT, &kbe);
			}""")

		if vars["SDL_INPUT_LINUXEV"] and arguments.with_haptic:
			vars["SDL_HAPTIC_LINUX"] = True
			product.sources += ["src/haptic/linux/*.c"]

		vars["HAVE_LIBUDEV_H"] = target_platform.check_include_exists("libudev.h")
		vars["HAVE_DBUS_DBUS_H"] = target_platform.check_include_exists("dbus/dbus.h")

		if arguments.with_joystick:
			logging.info("TODO: CheckUSBHID")
			vars["SDL_JOYSTICK_LINUX"] = True
			product.sources += ["src/joystick/linux/*.c"]


		check_pthread(arguments, product, target_platform, vars)

		# check clock_gettime
		have_clock_gettime = False
		if target_platform.check_function_exists("clock_gettime", includes=["time.h"], linkflags=["-lrt"]):
			product.links += ["rt"]
			have_clock_gettime = True
		elif target_platform.check_function_exists("clock_gettime", includes=["time.h"], linkflags=["-lc"]):
			product.links += ["c"]
			have_clock_gettime = True
		vars["HAVE_CLOCK_GETTIME"] = have_clock_gettime

		if arguments.with_filesystem:
			vars["SDL_FILESYSTEM_UNIX"] = True
			product.sources += ["src/filesystem/unix/*.c"]

		vars["HAVE_LINUX_VERSION_H"] = target_platform.check_include_exists("linux/version.h")

		if arguments.with_power:
			vars["SDL_POWER_LINUX"] = True
			product.sources += ["src/power/linux/*.c"]

		if arguments.with_timers:
			vars["SDL_TIMER_UNIX"] = True
			product.sources += ["src/timer/unix/*.c"]

		logging.info("TODO: check rpath")

		#product.linkflags += ["-Wl,-soname=libSDL2.so.2.0.2"]


	elif target_platform.matches("windows"):
		logging.info("TODO: support windows")
		windows = product.layout(platform="windows")

		if arguments.with_file:
			windows.sources += ["src/filesystem/windows/*.c"]

		if arguments.with_joystick:
			windows.sources += ["src/joystick/windows/*.c"]

		if arguments.with_haptic:
			windows.sources += ["src/haptic/windows/*.c"]

		if arguments.with_power:
			windows.sources += ["src/power/windows/*.c"]

		if arguments.with_timers:
			windows.sources += ["src/timer/windows/*.c"]

		if arguments.with_threads:
			windows.sources += ["src/thread/windows/*.c"]

	elif target_platform.matches("macosx"):
		# TODO: until I come up with a way to set this globally
		# We must set this here to match our application's setting.
		macosx = product.layout(platform="macosx")
		macosx.driver.macosx_deployment_target = "10.8"


		debug = product.layout(configuration="debug", platform="macosx")
		debug.driver.gcc_generate_debugging_symbols = "YES"
		debug.driver.debug_information_format = "dwarf-with-dsym"

		release = product.layout(configuration="release", platform="macosx")
		release.driver.gcc_generate_debugging_symbols = "NO"
		release.driver.gcc_enable_cpp_rtti = "NO"
		release.driver.gcc_strict_aliasing = "NO"
		release.driver.gcc_optimization_level = "3"


		core_audio_framework = False
		audio_unit_framework = False
		iokit_framework = False
		forcefeedback_framework = False
		carbon_framework = False
		core_video_framework = False

		if arguments.with_file:
			product.sources += ["src/file/cocoa/*.m"]

			product.links += ["Cocoa.framework"]
			vars["SDL_FRAMEWORK_COCOA"] = True
		else:
			raise Exception("SDL_FILE must be enabled to build on MacOS X")

		if arguments.with_audio:
			vars["SDL_AUDIO_DRIVER_COREAUDIO"] = True
			product.sources += ["src/audio/coreaudio/*.c"]
			core_audio_framework = True
			audio_unit_framework = True

		if arguments.with_joystick:
			vars["SDL_JOYSTICK_IOKIT"] = True
			product.sources += ["src/joystick/darwin/*.c"]
			iokit_framework = True
			forcefeedback_framework = True

		if arguments.with_haptic:
			logging.info("compiling with macosx haptic!")
			vars["SDL_HAPTIC_IOKIT"] = True
			product.sources += ["src/haptic/darwin/*.c"]
			iokit_framework = True
			forcefeedback_framework = True

			if not arguments.with_joystick:
				raise Exception("SDL_HAPTIC requires SDL_JOYSTICK to be enabled!")

		if arguments.with_power:
			vars["SDL_POWER_MACOSX"] = True
			product.sources += ["src/power/macosx/*.c"]
			carbon_framework = True
			iokit_framework = True

		if arguments.with_timers:
			vars["SDL_TIMER_UNIX"] = True
			product.sources += ["src/timer/unix/*.c"]

		if arguments.with_filesystem:
			vars["SDL_FILESYSTEM_COCOA"] = True
			product.sources += ["src/filesystem/cocoa/*.m"]


		# final step to add in frameworks
		if arguments.with_video:
			core_video_framework = True
			if arguments.video_cocoa:
				product.sources += ["src/video/cocoa/*.m"]
				vars["SDL_VIDEO_DRIVER_COCOA"] = True


			if arguments.video_opengl:
				vars["SDL_VIDEO_OPENGL"] = True
				vars["SDL_VIDEO_OPENGL_CGL"] = True
				vars["SDL_VIDEO_RENDER_OGL"] = True
				logging.info("TODO: Find library opengl and link")

		check_pthread(arguments, product, target_platform, vars)

		framework_value_map = {
			"CoreAudio.framework" : core_audio_framework,
			"AudioUnit.framework" : audio_unit_framework,
			"IOKit.framework" : iokit_framework,
			"ForceFeedback.framework" : forcefeedback_framework,
			"Carbon.framework" : carbon_framework,
			"CoreVideo.framework" : core_video_framework
		}

		for key, value in framework_value_map.iteritems():
			if value:
				product.links.append(key)


	if target_platform.matches("windows"):
		windows = product.layout(platform="windows")

		# check for directx
		check_directx(vars, arguments, windows, target_platform)

		windows.sources += [
			"src/core/windows/*.c",
		]

		windows.links += [
			"winmm",
			"imm32",
			"version",
			"kernel32"
		]

		product.sources += [
			"src/audio/winmm/*.c"
		]

		if vars["HAVE_DSOUND_H"]:
			product.sources += [
				"src/audio/directsound/*.c"
			]

		if vars["HAVE_XAUDIO2_H"]:
			logging.info("have xaudio2")
			product.sources += [
				"src/audio/xaudio2/*.c"
			]		


	return vars

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)


	# generate SDL_config.h from a template. This is disabled for iPhone, iPhoneSimulator, Windows
	generate_config = True

	product_type = ProductType.DynamicLibrary

	# temporarily use static lib on windows
	if target_platform.matches("windows"):
		product_type = ProductType.StaticLibrary

	sdl2 = Product(name="SDL2", output=product_type)
	sdl2.root = "../dependencies/SDL2"
	sdl2.includes = [
		"include"
	]

	# general source files
	sdl2.sources = [
		"src/*.c",
		"src/atomic/*.c",
		"src/events/*.c",
		"src/file/*.c",
		"src/libm/*.c",
		"src/stdlib/*.c",

		"include/*.h"
	]

	if not target_platform.matches("windows"):
		sdl2.sources += [
			"src/audio/*.c",
			"src/cpuinfo/*.c",
			"src/dynapi/*.c",
			"src/render/*.c",
			"src/render/**.c",
			"src/thread/*.c",
			"src/timer/*.c",
			"src/video/*.c"	
		]

	# We cannot disable these in Windows builds because we 
	# do not generate a config.
	if target_platform.matches("windows"):
		sdl2.sources += [
			"src/audio/*.c",
			"src/audio/disk/*.c",
			"src/audio/dummy/*.c",
			"src/dynapi/*.c",
			"src/render/*.c",
			"src/render/**.c",
			"src/thread/*.c",
			"src/timer/*.c",
			"src/video/*.c",
			"src/timer/windows/*.c",
			"src/thread/windows/*.c",
			"src/haptic/windows/*.c",
			"src/filesystem/windows/*.c",
			"src/power/windows/*.c",
			"src/joystick/windows/*.c",
			"src/loadso/windows/*.c",
			"src/video/windows/*.c"
		]

	sdl2.excludes = [
		"src/SDL_test*",
		"include/SDL_test*"
	]

	if arguments.with_cpuinfo:
		sdl2.sources += [
			"src/cpuinfo/*.c"
		]


	debug = sdl2.layout(configuration="debug")
	debug.defines = [
		"DEBUG"
	]

	release = sdl2.layout(configuration="release")
	release.defines = [
		"NDEBUG"
	]

	if target_platform.matches("macosx"):
		sdl2.defines += [
			"TARGET_API_MAC_OSX"
		]

		sdl2.links += [
			"iconv",
			"objc",
			"m",
			"OpenGL.framework",
			"AudioToolbox.framework",
			"AudioUnit.framework"
		]

	elif target_platform.matches("linux"):
		debug.defines += [
			"_THREAD_SAFE"
		]
		debug.cflags = [
			"-Wall",
			"-g",
			"-O0",
			"-fvisibility=hidden"
		]

		instruction_set = [
			"-mmmx",
			"-msse",
			"-msse2"
		]
		desktop = sdl2.layout(architecture="x86_64")
		desktop.cflags += instruction_set

		desktop = sdl2.layout(architecture="x86")
		desktop.cflags += instruction_set

		release.cflags = ["-O3"]
		release.linkflags = ["-s"]

	elif target_platform.matches("iphonesimulator") or target_platform.matches("iphone"):
		generate_config = False
		disable_shared = True
		enable_static = True

		debug.driver.gcc_optimization_level = "0"
		release.driver.gcc_optimization_level = "2"

		sdl2.cflags += [
			"-miphoneos-version-min=3.0",
			"-no-cpp-precomp",
			"-pipe",
		]

		sdl2.linkflags += [
			"-miphoneos-version-min=3.0",
			"-static-libgcc",
		]

		logging.info("Copy config setup!")
		sdl2.copy_config("include/SDL_config_iphoneos.h", "include/SDL_config.h")

	elif target_platform.matches("windows"):
		generate_config = False

	# TODO: For now this is called because it is performs multiple tasks
	# including adding source files to the product. This needs to be broken up.
	config_vars = get_config_variables(arguments, sdl2, target_platform)

	if generate_config:
		# create the SDL_config.h
		cfg = sdl2.create_config("include/SDL_config.h", template="include/SDL_config.h.cmake", variables=config_vars)

	return [sdl2]


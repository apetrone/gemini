import os
import logging

from pegasus.models import Product, ProductType, Dependency

DEPENDENCIES_FOLDER = "dependencies"
SDKS_FOLDER = os.path.join(DEPENDENCIES_FOLDER, "sdks")

DESKTOP = ["macosx", "linux", "windows"]
#BLACKSMITH_PATH = "../tools/blacksmith/blacksmith.py"
COMMON_PRODUCT_ROOT = "bin/${CONFIGURATION}_${ARCHITECTURE}"
COMMON_PROJECT_ROOT = "_projects"

# TODO:
	# windows:
	# - platform needs user32.lib
	# 


# dependencies
libsdl = Dependency(file="sdl2.py",
		arguments=[
		"--with-audio=0",
		"--with-render=0",
		#"--with-filesystem=0",
		#"--with-file=0", required on MacOSX.
		"--with-cpuinfo=0",
		#"--with-power=0"
	]
)

libnom = Dependency(file="nom.py")
librecastnavigation = Dependency(file="recastnavigation.py")

def setup_common_variables(arguments, target_platform, product):
	product.sources += [
		"src/engine/*.c*",
		"src/engine/*.h*",

		"src/engine/physics/*.c*",
		"src/engine/physics/*.h*",

		"src/engine/physics/bullet/*.c*",
		"src/engine/physics/bullet/*.h*",

		"src/sdk/include/**.h"
	]


	product.includes += [
		"src",
		"src/engine",
		"src/engine/game",

		"src/sdk/include"
	]

	# TODO: Allow generic *.DS_Store excludes
	product.excludes = [
		"src/engine/.DS_Store",
		"src/engine/assets/.DS_Store",
		"src/engine/contrib/.DS_Store",
		"src/engine/game/.DS_Store",
		#"src/engine/entry.cpp",

		# excluded, but not yet removed game crap
		"src/engine/kernels/test_universal.cpp",
		"src/engine/game/gamescreen.*",
		"src/engine/game/win_loss_screen.*",
		"src/engine/game/componentmanager.*",
		"src/engine/game/componentlibrary.*",
		"src/engine/game/components.h",
		"src/engine/game/menuscreen.*",
		"src/engine/game/helpscreen.*",
		"src/engine/game/logoscreen.*",
		"src/engine/game/screencontrol.*",

		# temporarily removing script/squirrel
		"src/engine/script.*",
		"src/engine/kernels/test_script.cpp",

		"src/engine/keyvalues.*"

	]

	index_type_map = {
		"uint" : "1",
		"ushort" : "2"
	}
	product.defines += [
		"PLATFORM_INDEX_TYPE=%s" % index_type_map[arguments.index_type]
	]


	debug = product.layout(configuration="debug")
	release = product.layout(configuration="release")

	debug.defines = [
		"DEBUG"
	]

def setup_datamodel(product):
	product.sources += [
		"src/tools/common.cpp",
		"src/tools/common.h",
		"src/tools/common/*.cpp",
		"src/tools/common/*.h",
		"src/tools/datamodel/*.cpp",
		"src/tools/datamodel/*.h"
	]

def setup_common_tool(product):

	product.root = "../"
	product.sources += [
		"src/tools/%s/*.cpp" % product.name,
		"src/tools/%s/*.h" % product.name
	]

	setup_datamodel(product)

	product.includes += [
		"src/tools/%s" % product.name,
		"src/tools/",
		"src/tools/common",
		"src/tools/datamodel"
	]


	product.defines += [
		"JSON_IS_AMALGAMATION"
	]

	product.dependencies += [
		Dependency(file="glm.py")
	]

	linux = product.layout(platform="linux")
	linux.links += [
		"pthread",
		"dl",
		#"asound"
	]
	linux.linkflags += [
		"-Wl,-rpath='$$$\ORIGIN'",
	]


def setup_common_libs(arguments, product):

	product.defines += [
		"JSON_IS_AMALGAMATION"
	]

	product.sources += [
		# dependencies
		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c")
	]

	product.includes += [
		"src/engine",
		"src/engine/audio",
		"src/contrib/stb_vorbis.c",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3")
	]


	product.dependencies += [
		Dependency(file="glm.py")
	]

	if arguments.with_civet:
		product.defines += [
			"USE_WEBSERVER=1"
		]

		product.sources += [
			os.path.join(DEPENDENCIES_FOLDER, "civetweb/src/CivetServer.cpp"),
			os.path.join(DEPENDENCIES_FOLDER, "civetweb/src/civetweb.c"),			
			os.path.join(DEPENDENCIES_FOLDER, "civetweb/include/*.h")
		]

		product.includes += [
			os.path.join(DEPENDENCIES_FOLDER, "civetweb/include")
		]

	linux = product.layout(platform="linux")
	linux.cflags += [
		"-Wpedantic"
	]
	linux.links += [
		"pthread",
		"dl",
		"asound" # TODO: need to detect ALSA
	]
	linux.linkflags += [
		"-Wl,-rpath='$$$\ORIGIN'",
		"-Wl,--unresolved-symbols=ignore-in-shared-libs"
	]


	windows = product.layout(platform="windows")
	windows.includes += [
		os.path.join(SDKS_FOLDER, "openal-1.1", "include")	
	]

	win32 = product.layout(platform="windows", architecture="x86")
	win32.libdirs += [ 
		os.path.join(SDKS_FOLDER, "openal-1.1", "libs/Win32")
	]

	win64 = product.layout(platform="windows", architecture="x86_64")
	win64.libdirs += [ 
		os.path.join(SDKS_FOLDER, "openal-1.1", "libs/Win64")
	]

	# currently, we use MultiByte character set (and with Squirrel3)
	# to avoid Unicode. This is a problem with squirrel that should be
	# addressed to use utf8 internally.
	windows.driver.characterset = "MultiByte"

def setup_driver(product):

	#macosx = product.layout(platform="macosx")
	#macosx.driver.macosx_deployment_target = "10.8"
	#macosx.driver.sdkroot = "macosx10.9"

	product.excludes += [
		"*.DS_Store"
	]

	mac_debug = product.layout(platform="macosx", configuration="debug")
	mac_debug.driver.gcc_optimization_level="0"
	mac_debug.driver.debug_information_format="dwarf-with-dsym"

	mac_debug.cflags += [
		"-Wall",
		"-Wmost",
		"-Weverything"
	]
	
	mac_release = product.layout(platform="macosx", configuration="release")


	linux = product.layout(platform="linux")
	linux.cflags += [
		"-fPIC"
	]

def get_tools(libruntime, librenderer, libplatform, libcore, **kwargs):
	#
	#
	#
	tools = []

	target_platform = kwargs.get("target_platform")
	
	#
	# muse: asset conversion tool
	#
	linux_arch_map = {
		"x86_64": "x64",
		"x86": "x86"
	}
	windows_arch_map = {
		"x86_64": "x64",
		"x86" : "x86"
	}
	libfbx_roots = {
		"macosx": "lib/clang/${CONFIGURATION}",
		"linux": "lib/gcc4/x64/${CONFIGURATION}",
		"windows": "lib/vs2013/x64/${CONFIGURATION}" # TODO: bugger fix this!
	}
	libfbx_names = {
		"macosx": "libfbxsdk",
		"linux": "fbxsdk",
		"windows": "libfbxsdk-md"
	}

	libfbx = Product(name=libfbx_names[target_platform.name], output=ProductType.DynamicLibrary)
	libfbx.root = os.path.join("..", SDKS_FOLDER, "fbx_2015.1")
	libfbx.includes = [
		"include"
	]

	libfbx.product_root = libfbx_roots[target_platform.name]
	tools.append(libfbx)

	muse = Product(name="muse", output=ProductType.Commandline)
	muse.project_root = COMMON_PROJECT_ROOT
	muse.dependencies.extend([
		libruntime,
		libplatform,
		libcore,
		libfbx
	])

	macosx = muse.layout(platform="macosx")
	macosx.links += [
		"Cocoa.framework"	
	]
	muse.product_root = COMMON_PRODUCT_ROOT
	setup_driver(muse)
	setup_common_tool(muse)
	tools.append(muse)

	kraken = get_kraken(arguments, libruntime, libplatform, libcore, librenderer, **kwargs)
	tools.append(kraken)

	orion = get_orion(arguments, libruntime, libplatform, libcore, librenderer, **kwargs)
	tools.append(orion)

	return tools


def get_libcore(arguments, target_platform):
	libcore = Product(name="core", output=ProductType.DynamicLibrary)
	setup_driver(libcore)
	libcore.project_root = COMMON_PROJECT_ROOT
	libcore.root = "../"
	libcore.sources += [
		"src/core/*.*",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c")
	]


	libcore.includes += [
		"src",
		"src/core",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3")
	]

	libcore.dependencies += [
		Dependency(file="glm.py")
	]

	return libcore
	
def get_libplatform(arguments, target_platform):
	libplatform = Product(name="platform", output=ProductType.DynamicLibrary)
	setup_driver(libplatform)
	libplatform.project_root = COMMON_PROJECT_ROOT
	libplatform.root = "../"
	libplatform.sources += [
		"src/platform/config.h",
		"src/platform/kernel.cpp",
		"src/platform/kernel.h",
		"src/platform/kernel_events.h",	
		"src/platform/mem.cpp",
		"src/platform/mem.h",
		"src/platform/mem_stl_allocator.h",
		"src/platform/platform.cpp",
		"src/platform/platform.h",
		"src/platform/platform_internal.h",
		"src/platform/input.cpp",
		"src/platform/input.h",
		"src/platform/windowlibrary.cpp",
		"src/platform/windowlibrary.h",
		"src/platform/sdl_windowlibrary.cpp",
		"src/platform/sdl_windowlibrary.h"
	]

	libplatform.includes += [
		"src/platform"
	]

	macosx = libplatform.layout(platform="macosx")
	macosx.sources += [
		# application
		"src/platform/application/osx/osx_appdelegate.mm",
		"src/platform/application/osx/osx_appdelegate.h",
		"src/platform/application/osx/osx_application.mm",
		"src/platform/application/osx/osx_application.h",

		# dylib
		"src/platform/dylib/osx/osx_dylib.cpp",
		"src/platform/dylib/posix/posix_dlopen.cpp",

		# filesystem
		"src/platform/filesystem/osx/osx_filesystem.mm",
		"src/platform/filesystem/posix/posix_filesystem_common.cpp",

		# os
		"src/platform/os/osx/osx_os.mm",

		# serial
		"src/platform/serial/posix/posix_serial.cpp",

		# thread
		"src/platform/thread/osx/osx_thread.cpp",
		"src/platform/thread/posix/posix_thread_common.cpp",
		"src/platform/thread/posix/posix_thread.h",
		
		# time
		"src/platform/time/osx/osx_timer.cpp",
		"src/platform/time/posix/posix_datetime.cpp"
	]

	macosx.includes += [
		"src/platform/posix"
	]

	macosx.links += [
		"Cocoa.framework"
	]


	linux = libplatform.layout(platform="linux")
	linux.sources += [
		# dylib
		"src/platform/dylib/posix/posix_dylib.cpp",
		"src/platform/dylib/posix/posix_dlopen.cpp",

		# filesystem
		"src/platform/filesystem/posix/posix_filesystem.cpp",
		"src/platform/filesystem/posix/posix_filesystem_common.cpp",

		# os
		"src/platform/os/posix/posix_os.cpp",

		# serial
		"src/platform/serial/posix/posix_serial.cpp",

		# thread
		"src/platform/thread/posix/posix_thread.cpp",
		"src/platform/thread/posix/posix_thread_common.cpp",
		"src/platform/thread/posix/posix_thread.h",

		# time
		"src/platform/time/posix/posix_datetime.cpp",
		"src/platform/time/posix/posix_timer.cpp"
	]

	linux.includes += [
		"src/platform/posix"
	]


	windows = libplatform.layout(platform="windows")
	windows.sources += [
		# dylib
		"src/platform/dylib/windows/win32_dylib.cpp",

		# filesystem
		"src/platform/filesystem/windows/win32_filesystem.cpp",

		# os
		"src/platform/os/windows/win32_os.cpp",

		# serial
		"src/platform/serial/win32/win32_serial.cpp",

		# thread
		"src/platform/thread/windows/win32_thread.cpp",
		"src/platform/thread/windows/windows_thread.h",

		# time
		"src/platform/time/windows/win32_time.cpp"
	]

	return libplatform



def get_librenderer(arguments, target_platform, libruntime):
	librenderer = Product(name="renderer", output=ProductType.DynamicLibrary)
	setup_driver(librenderer)
	librenderer.project_root = COMMON_PROJECT_ROOT
	librenderer.root = "../"
	librenderer.sources += [
		"src/renderer/*.*",
		"src/renderer/gl/*.cpp",
		"src/renderer/gl/*.h",

		"src/contrib/stb_image.c",
		"src/contrib/stb_truetype.h",		

		# include this almagamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp")		
	]

	librenderer.defines += [
		"JSON_IS_AMALGAMATION"
	]

	librenderer.includes += [
		"src/contrib",

		"src/renderer",
		"src/renderer/gl",

		os.path.join(DEPENDENCIES_FOLDER, "fontstash/src"),
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp")	
	]

	librenderer.dependencies += [
		libruntime
	]

	if target_platform.get() in DESKTOP:
		librenderer.sources += [
			"src/renderer/gl/desktop/*.cpp",
			"src/renderer/gl/desktop/*.h"
		]


	if arguments.gles:
		librenderer.sources += [
			"src/renderer/gl/mobile/*.cpp",
			"src/renderer/gl/mobile/*.h"			
		]

		librenderer.defines += [
			"PLATFORM_USE_GLES2=1"
		]

	macosx = librenderer.layout(platform="macosx")
	macosx.sources += [
		"src/renderer/gl/gemgl_osx.mm"
	]

	macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	return librenderer

def get_libruntime(arguments, target_platform):
	libruntime = Product(name="runtime", output=ProductType.DynamicLibrary)
	setup_driver(libruntime)
	libruntime.project_root = COMMON_PROJECT_ROOT
	libruntime.root = "../"
	libruntime.sources += [
		"src/runtime/*.c",
		"src/runtime/*.cpp",
		"src/runtime/*.h",

		# include this almagamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp")		
	]

	libruntime.defines += [
		"JSON_IS_AMALGAMATION"
	]

	libruntime.includes += [
		"src/runtime",
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp"),		
	]

	libruntime.dependencies.append(libnom)

	return libruntime

def get_rnd(arguments, links, **kwargs):
	global_params = kwargs.get("global_params")
	target_platform = kwargs.get("target_platform")

	rnd = Product(name="rnd", output=ProductType.Commandline)
	rnd.project_root = COMMON_PROJECT_ROOT
	rnd.root = "../"
	rnd.sources += [
		"src/rnd/rnd.cpp"
	]
	rnd.product_root = COMMON_PRODUCT_ROOT

	setup_driver(rnd)
	setup_common_tool(rnd)

	rnd.dependencies.append(libsdl)
	rnd.dependencies.extend(links)

	rnd_macosx = rnd.layout(platform="macosx")
	rnd_macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	rnd_linux = rnd.layout(platform="linux")
	rnd_linux.links += [
		"GL"
	]

	# Fix undefined reference for SysFreeString
	rnd_windows = rnd.layout(platform="windows")
	rnd_windows.links += [
		"OpenGL32",
		#"OpenAL32",
		"gdi32",
		"winspool",
		"comdlg32",
		"advapi32",
		"shell32",
		"ole32",
		"user32",
		"oleaut32"
	]

	return rnd

def create_unit_test(name, links, source):
	product = Product(name=name, output=ProductType.Commandline)	
	product.project_root = COMMON_PROJECT_ROOT
	product.root = "../"	
	product.includes += [
		"tests"
	]
	product.sources += [
		"tests/unit_test.h",
		source
	]
	product.dependencies.extend(links)
	return product

def get_unit_tests(arguments, links, **kwargs):
	return [
		create_unit_test("test_core", links, "tests/src/test_core.cpp"),
		create_unit_test("test_platform", links, "tests/src/test_platform.cpp"),
		create_unit_test("test_runtime", links, "tests/src/test_runtime.cpp")
	]

def get_kraken(arguments, libruntime, librenderer, libplatform, libcore, **kwargs):
	global_params = kwargs.get("global_params")
	target_platform = kwargs.get("target_platform")

	kraken = Product(name="kraken", output=ProductType.Application)
	kraken.project_root = COMMON_PROJECT_ROOT
	kraken.product_root = COMMON_PRODUCT_ROOT
	kraken.root = "../"
	#kraken.sources += [
	#]

	setup_driver(kraken)
	setup_common_tool(kraken)

	kraken.dependencies.extend([
		libsdl,
		libplatform,
		libcore,
		librenderer,
		libnom
	])

	kraken.sources += [
		"src/tools/kraken/kraken.cpp"	
	]

	macosx = kraken.layout(platform="macosx")
	macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	# TODO: This path is relative to the *project*
	macosx.driver.infoplist_file = "../src/tools/kraken/resources/osx/Info.plist"
	macosx.resources = [
		"src/tools/kraken/resources/osx/en.lproj/*.xib",
		"src/tools/kraken/resources/osx/en.lproj/*.strings"
	]	

	linux = kraken.layout(platform="linux")
	linux.links += [
		"GL"
	]

	return kraken

def get_orion(arguments, libruntime, libplatform, libcore, librenderer, **kwargs):
	orion = Product(name="orion", output=ProductType.Application)
	orion.project_root = COMMON_PROJECT_ROOT
	orion.product_root = COMMON_PRODUCT_ROOT
	orion.root = "../"

	setup_driver(orion)
	setup_common_tool(orion)

	orion.dependencies.extend([
		libsdl,
		libplatform,
		libcore,
		librenderer,
		libruntime,		
		Dependency(file="nom.py")
	])

	orion.sources += [
		"src/tools/orion/orion.cpp"	
	]

	macosx = orion.layout(platform="macosx")
	macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	# TODO: This path is relative to the *project*
	macosx.driver.infoplist_file = "../src/tools/orion/resources/osx/Info.plist"
	macosx.resources = [
		"src/tools/orion/resources/osx/en.lproj/*.xib",
		"src/tools/orion/resources/osx/en.lproj/*.strings"
	]	

	linux = orion.layout(platform="linux")
	linux.links += [
		"GL"
	]

	linux.cflags += [
		"-fPIC"
	]		

	return orion

def arguments(parser):
	parser.add_argument("--with-gles", dest="gles", action="store_true", help="Build with GLES support", default=False)
	parser.add_argument("--raspberrypi", dest="raspberrypi", action="store_true", help="Build for the RaspberryPi", default=False)
	
	parser.add_argument("--indextype", dest="index_type", choices=["uint", "ushort"], type=str, default="uint", help="Set the IndexBuffer type; defaults to uint")

	parser.add_argument("--with-civet", dest="with_civet", action="store_true", help="Build with CivetServer", default=True)
	parser.add_argument("--no-civet", dest="with_civet", action="store_false", help="Build without CivetServer")

	parser.add_argument("--with-oculusvr", dest="with_oculusvr", action="store_true", help="Build with OculusVR support", default=False)
	parser.add_argument("--with-tools", dest="with_tools", action="store_true", help="Build with support for tools", default=False)
	parser.add_argument("--with-tests", dest="with_tests", action="store_true", help="Build with support for unit tests", default=False)

def products(arguments, **kwargs):

	# warn about SDKs...
	if arguments.with_oculusvr or arguments.with_tools:
		logging.warn("Generating projects for products which require private SDKs...")
		logging.warn("Please be sure you've run build/scripts/sync_sdks.py!")
		if not os.path.exists(os.path.join("build", SDKS_FOLDER)):
			raise Exception("SDKs folder is missing!")

	# global params will be inherited by all dependent products
	global_params = kwargs.get("global_params")

	g_macosx = global_params.layout(platform="macosx")

	# use C++11
	g_macosx.driver.clang_cxx_language_standard = "c++0x"

	# use LLVM C++ standard library with C++11 support
	g_macosx.driver.clang_cxx_library = "libc++"
	g_macosx.driver.macosx_deployment_target = "10.8"
	g_macosx.driver.sdkroot = "macosx10.9"

	# make visibility conformant
	g_macosx.driver.gcc_inlines_are_private_extern = "NO"
	g_macosx.driver.gcc_symbols_private_extern = "NO"

	# make sure we get smybols for libs
	debug_mac = global_params.layout(platform="macosx", configuration="debug")
	debug_mac.driver.gcc_optimization_level = "0"
	debug_mac.driver.debug_information_format = "dwarf-with-dsym"


	#g_linux = global_params.layout(platform="linux")
	# use C++11
	#g_linux.driver.cxxflags = ["-std=c++0x"]


	target_platform = kwargs.get("target_platform")

	libcore = get_libcore(arguments, target_platform)

	libplatform = get_libplatform(arguments, target_platform)
	libplatform.dependencies += [libsdl, libcore]

	libruntime = get_libruntime(arguments, target_platform)
	libruntime.dependencies += [libcore, libplatform, Dependency(file="glm.py")]

	librenderer = get_librenderer(arguments, target_platform, libruntime)
	librenderer.dependencies += [libcore, libplatform, Dependency(file="glm.py")]

	# don't add this until we clean up the shaderconfig dependency on libruntime
	#libruntime.dependencies.append(librenderer)
	


	tools = []
	if arguments.with_tools:
		tools = get_tools(libruntime, librenderer, libplatform, libcore, **kwargs)
	else:
		logging.warn("Compiling WITHOUT tools...")

	gemini = Product(name="gemini_desktop", output=ProductType.Application)
	gemini.project_root = COMMON_PROJECT_ROOT
	gemini.root = "../"
	gemini.product_root = COMMON_PRODUCT_ROOT
	gemini.object_root = "obj"

	setup_common_libs(arguments, gemini)

	gemini.sources += [
		"src/engine/gemini.cpp"
	]

	gemini.dependencies += [
		libruntime,
		librenderer,
		libplatform,
		libsdl,
		libcore,
		Dependency(file="sqrat.py"),
		#Dependency(file="squirrel3.py", products=["squirrel", "sqstdlib"]),
		libnom,
		Dependency(file="bullet2.py", products=["BulletSoftBody", "BulletDynamics", "BulletCollision", "LinearMath"]),
		librecastnavigation
	]

	if arguments.with_oculusvr:
		gemini.dependencies += [
			Dependency(file="oculusvr.py")
		]

		gemini.defines += [
			"GEMINI_WITH_OCULUSVR=1"
		]

	else:
		gemini.excludes += [
			"src/kernels/test_oculusvr.cpp"
		]

	# common sources
	setup_common_variables(arguments, target_platform, gemini)

	setup_driver(gemini)

	# more sources
	gemini.sources += [
		"src/engine/*.*",
		"src/engine/audio/openal.*",
		"src/engine/assets/*.*",
		"src/engine/game/**.*",
		"src/contrib/*",
	]

	gemini.excludes += [
		"*.DS_Store",
		"src/engine/kernels/test_assimp.cpp",
		"src/engine/kernels/project_huckleberry.cpp",

		"src/engine/kernels/test_bullet2.cpp",
		"src/engine/kernels/test_mobile.cpp",
		"src/engine/kernels/test_nom.cpp"
	]



	

	if target_platform.get() in DESKTOP:
		gemini.sources += [
			"src/engine/audio/openal_vorbis_decoder.*"
		]

		gemini.includes += [
			"src/engine/audio"
		]

		macosx = gemini.layout(platform="macosx")
		macosx.links = [
			"Cocoa.framework",
			"OpenGL.framework",
			"AudioToolbox.framework",
			"OpenAL.framework"
		]

		# TODO: This path is relative to the *project*
		macosx.driver.infoplist_file = "../src/engine/resources/osx/Info.plist"
		macosx.resources = [
			"src/engine/resources/osx/en.lproj/*.xib",
			"src/engine/resources/osx/en.lproj/*.strings"
		]

		linux = gemini.layout(platform="linux")

		linux.links += [
			"openal"
		]

		if arguments.raspberrypi:
			linux.defines += [
				"PLATFORM_RASPBERRYPI=1"
			]

			linux.links += ["X11"]

		if arguments.gles:

			linux.defines += [
				"PLATFORM_USE_GLES=1"
			]			
		else:
			linux.links += ["GL"]


		windows = gemini.layout(platform="windows")
		windows.links += [
			"OpenGL32",
			"OpenAL32",
			"gdi32",
			"winspool",
			"comdlg32",
			"advapi32",
			"shell32",
			"ole32",
			"user32"
		]
		
	iphoneos = gemini.layout(platform="iphoneos")
	# iphoneos.prebuild_commands += [
	# 	"python %s -c ../assets/ios.conf -y" % (BLACKSMITH_PATH)
	# ]

	iphoneos.sources += [
		"src/engine/audio/audio_extaudio_decoder.*"
	]

	iphoneos.resources = [
		"resources/ios/Settings.bundle",
		"resources/ios/en.lproj/*.*"
	]


	rnd = get_rnd(arguments, [libruntime, librenderer, libplatform, libcore], **kwargs)
	tests = []
	if arguments.with_tests:
		# Ugh, for now, we also link in libnom because the runtime requires it.
		# I feel like pegasus should identify such dependencies and take care of it in the future.
		# Though, for now, just link it in.
		tests = get_unit_tests(arguments, [libruntime, librenderer, libplatform, libcore, Dependency(file="glm.py"), libnom], **kwargs)

	return [librenderer, libruntime, libplatform, libcore] + [gemini] + tools + [rnd] + tests


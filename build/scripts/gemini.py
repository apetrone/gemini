import os
import logging

from pegasus.models import Product, ProductType, Dependency

DEPENDENCIES_FOLDER = "dependencies"
DESKTOP = ["macosx", "linux", "windows"]
#BLACKSMITH_PATH = "../tools/blacksmith/blacksmith.py"
COMMON_PRODUCT_ROOT = "bin/${CONFIGURATION}_${ARCHITECTURE}"

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

def setup_common_variables(arguments, target_platform, product):
	product.sources += [
		"src/engine/*.c*",
		"src/engine/*.h*"
	]


	product.includes += [
		"src",
		"src/engine",
		"src/engine/game"
	]

	# TODO: Allow generic *.DS_Store excludes
	product.excludes = [
		"src/engine/.DS_Store",
		"src/engine/assets/.DS_Store",
		"src/engine/contrib/.DS_Store",
		"src/engine/game/.DS_Store",
		"src/engine/entry.cpp",

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
		"src/engine/game/screencontrol.*"
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

def setup_common_tool(product):

	product.root = "../"
	product.sources += [
		"src/tools/%s/*.cpp" % product.name,
		"src/tools/%s/*.h" % product.name,
		"src/tools/*.cpp",
		"src/tools/*.h",
		"src/tools/common/*.cpp",
		"src/tools/common/*.h",
		"src/tools/datamodel/*.cpp",
		"src/tools/datamodel/*.h"
	]

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
		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c"),
		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.c"),
		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.h")
	]

	product.includes += [
		"src/engine",
		"src/engine/audio",
		"src/contrib",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3"),
		os.path.join(DEPENDENCIES_FOLDER, "slim")
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
		os.path.join(DEPENDENCIES_FOLDER, "openal-1.1", "include")	
	]

	win32 = product.layout(platform="windows", architecture="x86")
	win32.libdirs += [ 
		os.path.join(DEPENDENCIES_FOLDER, "openal-1.1", "libs/Win32")
	]

	win64 = product.layout(platform="windows", architecture="x86_64")
	win64.libdirs += [ 
		os.path.join(DEPENDENCIES_FOLDER, "openal-1.1", "libs/Win64")
	]

	# currently, we use MultiByte character set (and with Squirrel3)
	# to avoid Unicode. This is a problem with squirrel that should be
	# addressed to use utf8 internally.
	windows.driver.characterset = "MultiByte"

def setup_driver(product):

	#macosx = product.layout(platform="macosx")
	#macosx.driver.macosx_deployment_target = "10.8"
	#macosx.driver.sdkroot = "macosx10.9"

	mac_debug = product.layout(platform="macosx", configuration="debug")
	mac_debug.driver.gcc_optimization_level="0"
	mac_debug.driver.debug_information_format="dwarf-with-dsym"
	
	mac_release = product.layout(platform="macosx", configuration="release")

def get_tools(target_platform, libplatform, libcore):
	#
	#
	#
	tools = []

	rnd = Product(name="rnd", output=ProductType.Commandline)

	rnd.root = "../"
	rnd.sources += [
		"src/rnd/rnd.cpp"
	]
	rnd.product_root = COMMON_PRODUCT_ROOT

	setup_driver(rnd)
	setup_common_tool(rnd)

	rnd.dependencies.extend([
		libsdl
	])

	rnd_macosx = rnd.layout(platform="macosx")
	rnd_macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	rnd_linux = rnd.layout(platform="linux")
	rnd_linux.links += [
		"GL"
	]

	#tools.append(rnd)
	
	#
	# muse: asset conversion tool
	#
	linux_arch_map = {
		"x86_64": "x64",
		"x86": "x86"
	}
	libfbx_roots = {
		"macosx": "lib/clang/${CONFIGURATION}",
		"linux": "lib/gcc4/x64/${CONFIGURATION}",
		"windows": "lib/vs2013/${ARCHITECTURE}/${CONFIGURATION}"
	}
	libfbx_names = {
		"macosx": "libfbxsdk",
		"linux": "fbxsdk",
		"windows": "libfbxsdk-md"
	}

	libfbx = Product(name=libfbx_names[target_platform.name], output=ProductType.DynamicLibrary)
	libfbx.root = "../dependencies/fbx_2015.1"
	libfbx.includes = [
		"include"
	]

	libfbx.product_root = libfbx_roots[target_platform.name]
	tools.append(libfbx)

	muse = Product(name="muse", output=ProductType.Commandline)
	muse.dependencies.extend([
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

	return tools

def get_librenderer(arguments, target_platform):
	librenderer = Product(name="renderer", output=ProductType.StaticLibrary)
	librenderer.root = "../"
	librenderer.sources += [
		"src/renderer/*.*",
		"src/renderer/gldrivers/opengl_common.*"
	]

	librenderer.defines += [
		"JSON_IS_AMALGAMATION"
	]

	librenderer.includes += [
		"src/contrib",

		"src/renderer",
		"src/renderer/gldrivers",

		os.path.join(DEPENDENCIES_FOLDER, "fontstash/src")		
	]

	librenderer.excludes += [
		"*.DS_Store"	
	]


	if target_platform.get() in DESKTOP:
		librenderer.sources += [
			"src/renderer/gldrivers/opengl_core32.*"
		]


	if arguments.glesv2:
		librenderer.sources += [
			"src/renderer/gldrivers/opengl_glesv2.*"
		]


	macosx = librenderer.layout(platform="macosx")
	macosx.sources += [
		"src/renderer/osx/osx_gemgl.*"
	]

	return librenderer

def get_libplatform(arguments, target_platform):
	libplatform = Product(name="platform", output=ProductType.StaticLibrary)
	libplatform.root = "../"
	libplatform.sources += [
		"src/platform/*.*"
	]

	libplatform.includes += [
		"src/platform"
	]

	libplatform.excludes += [
		"*.DS_Store"
	]

	macosx = libplatform.layout(platform="macosx")
	macosx.sources += [
		"src/platform/osx/*.*"
	]


	linux = libplatform.layout(platform="linux")
	linux.sources += [
		"src/platform/linux/*.*"
	]

	windows = libplatform.layout(platform="windows")
	windows.sources += [
		"src/platform/windows/*.*"
	]

	return libplatform

def get_libcore(arguments, target_platform):
	libcore = Product(name="core", output=ProductType.StaticLibrary)
	libcore.root = "../"
	libcore.sources += [
		"src/core/*.*",

		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.c"),
		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.h"),

		# include this almagamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp")
	]

	libcore.defines += [
		"JSON_IS_AMALGAMATION"
	]

	libcore.includes += [
		"src",
		"src/core",

		os.path.join(DEPENDENCIES_FOLDER, "slim"),

		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp")
	]

	libcore.excludes += [
		"*.DS_Store"
	]

	return libcore

def arguments(parser):
	parser.add_argument("--with-glesv2", dest="glesv2", action="store_true", help="Build with GLES V2", default=False)
	parser.add_argument("--raspberrypi", dest="raspberrypi", action="store_true", help="Build for the RaspberryPi", default=False)
	
	parser.add_argument("--indextype", dest="index_type", choices=["uint", "ushort"], type=str, default="uint", help="Set the IndexBuffer type; defaults to uint")

	parser.add_argument("--with-civet", dest="with_civet", action="store_true", help="Build with CivetServer", default=True)
	parser.add_argument("--no-civet", dest="with_civet", action="store_false", help="Build without CivetServer")

	parser.add_argument("--with-oculusvr", dest="with_oculusvr", action="store_true", help="Build with OculusVR support", default=True)
	parser.add_argument("--no-oculusvr", dest="with_oculusvr", action="store_false", help="Build without OculusVR support")

def products(arguments, **kwargs):
	# global params will be inherited by all dependent products
	global_params = kwargs.get("global_params")

	g_macosx = global_params.layout(platform="macosx")

	# use C++11
	g_macosx.driver.clang_cxx_language_standard = "c++0x"

	# use LLVM C++ standard library with C++11 support
	g_macosx.driver.clang_cxx_library = "libc++"
	g_macosx.driver.macosx_deployment_target = "10.8"
	g_macosx.driver.sdkroot = "macosx10.9"

	# make sure we get smybols for libs
	debug_mac = global_params.layout(platform="macosx", configuration="debug")
	debug_mac.driver.gcc_optimization_level = "0"
	debug_mac.driver.debug_information_format = "dwarf-with-dsym"


	#g_linux = global_params.layout(platform="linux")
	# use C++11
	#g_linux.driver.cxxflags = ["-std=c++0x"]


	target_platform = kwargs.get("target_platform")

	gemini = Product(name="gemini_desktop", output=ProductType.Application)
	gemini.root = "../"
	gemini.product_root = COMMON_PRODUCT_ROOT
	gemini.object_root = "obj"

	setup_common_libs(arguments, gemini)

	gemini.dependencies += [
		Dependency(file="sqrat.py"),
		Dependency(file="squirrel3.py", products=["squirrel", "sqstdlib"]),
		Dependency(file="nom.py"),
		Dependency(file="bullet3.py", products=["BulletSoftBody", "BulletDynamics", "BulletCollision", "LinearMath"])
		#Dependency(file="box2d.py")
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
		"src/engine/kernels/**.c*",
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
			"src/engine/platforms/desktop/kernel_desktop.cpp",
			"src/engine/audio/openal_vorbis_decoder.*"
		]

		gemini.includes += [
			"src/engine/audio"
		]

		#gemini.prebuild_commands = [
		#	"python %s -c ../assets/desktop.conf" % (BLACKSMITH_PATH)
		#]

		xwl_params = []
		if target_platform.get() == "linux":
			xwl_params.append("--with-x11")
			#xwl_params.append("--with-egl")

		gemini.dependencies += [
			libsdl
		]

		macosx = gemini.layout(platform="macosx")
		macosx.sources = [
			"src/engine/platforms/osx/*.m*",
			"src/engine/platforms/osx/*.h*"
		]
		macosx.links = [
			"Cocoa.framework",
			"OpenGL.framework",
			"AudioToolbox.framework",
			"OpenAL.framework"
		]

		macosx.driver.infoplist_file = "src/engine/resources/osx/Info.plist"
		macosx.resources = [
			"src/engine/resources/osx/en.lproj/*.xib",
			"src/engine/resources/osx/en.lproj/*.strings"
		]



		linux = gemini.layout(platform="linux")
		linux.sources += [
			"src/engine/platforms/desktop/entry.cpp"
		]

		linux.links += [
			"openal"
		]

		if arguments.raspberrypi:
			linux.defines += [
				"PLATFORM_RASPBERRYPI=1"
			]

			linux.links += ["X11"]

		if arguments.glesv2:

			linux.defines += [
				"PLATFORM_USE_GLES2=1"
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

		windows.sources += [
			"src/engine/platforms/desktop/entry_windows.cpp"
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

	libplatform = get_libplatform(arguments, target_platform)

	libcore = get_libcore(arguments, target_platform)
	libcore.dependencies += [libplatform, Dependency(file="glm.py")]

	librenderer = get_librenderer(arguments, target_platform)
	librenderer.dependencies += [Dependency(file="glm.py"), libplatform, libcore]

	tools = get_tools(target_platform, libplatform, libcore)

	gemini.dependencies.extend([librenderer, libcore, libplatform])

	return [gemini] + tools + [librenderer, libcore, libplatform]


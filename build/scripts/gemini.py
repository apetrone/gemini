import os
import logging

from pegasus.models import Product, ProductType, Dependency

DEPENDENCIES_FOLDER = "dependencies"
DESKTOP = ["macosx", "linux", "windows"]
BLACKSMITH_PATH = "../tools/blacksmith/blacksmith.py"
COMMON_PRODUCT_ROOT = "latest/bin/${CONFIGURATION}_${ARCHITECTURE}"

def setup_common_variables(arguments, target_platform, product):
	product.sources += [
		"src/*.c*",
		"src/*.h*"
	]


	product.includes += [
		"src",
		"src/game"
	]

	# TODO: Allow generic *.DS_Store excludes
	product.excludes = [
		"src/core/.DS_Store",
		"src/core/assets/.DS_Store",
		"src/contrib/.DS_Store",
		"src/game/.DS_Store",
		"src/entry.cpp",

		# excluded, but not yet removed game crap
		"src/kernels/test_universal.cpp",
		"src/game/gamescreen.*",
		"src/game/win_loss_screen.*",
		"src/game/componentmanager.*",
		"src/game/componentlibrary.*",
		"src/game/components.h",
		"src/game/menuscreen.*",
		"src/game/helpscreen.*",
		"src/game/logoscreen.*",
		"src/game/screencontrol.*",

		"src/kernels/test_bullet2.cpp"
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

def setup_common_libs(product):

	product.defines += [
		"JSON_IS_AMALGAMATION"
	]

	product.sources += [
		# dependencies
		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c"),

		# include this almagamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp"),

		os.path.join(DEPENDENCIES_FOLDER, "font-stash/fontstash.c"),
		os.path.join(DEPENDENCIES_FOLDER, "font-stash/stb_truetype.c"),

		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.c"),
		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.h"),


		"src/util.cpp",
		"src/core/mem.cpp"
	]

	product.includes += [
		"src",
		"src/core",
		"src/core/audio",
		"src/contrib",
		"src/sdk",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3"),
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp"),
		os.path.join(DEPENDENCIES_FOLDER, "font-stash"),
		os.path.join(DEPENDENCIES_FOLDER, "slim")		
	]


	product.dependencies += [
		Dependency(file="glm.py")
	]

	linux = product.layout(platform="linux")
	linux.links += [
		"pthread",
		"dl",
		"asound"
	]
	linux.linkflags += [
		"-Wl,-rpath,."
	]	

def setup_driver(product):

	#macosx = product.layout(platform="macosx")
	#macosx.driver.macosx_deployment_target = "10.8"
	#macosx.driver.sdkroot = "macosx10.9"

	mac_debug = product.layout(platform="macosx", configuration="debug")
	mac_debug.driver.gcc_optimization_level="0"
	mac_debug.driver.debug_information_format="dwarf-with-dsym"
	
	mac_release = product.layout(platform="macosx", configuration="release")

def get_tools():
	#
	#
	#

	# rnd = Product(name="rnd", output=ProductType.Commandline)

	# rnd.root = "../"
	# rnd.sources += [
	# 	"src/rnd/rnd.cpp"
	# ]
	# rnd.product_root = COMMON_PRODUCT_ROOT

	# setup_driver(rnd)
	# setup_common_libs(rnd)

	#
	# other tools?
	# 
	# 
	prism = Product(name="prism", output=ProductType.Commandline)

	prism.root = "../"
	prism.sources += [
		"src/tools/prism/**.cpp"
	]
	prism.includes += [
		"src/sdk"
	]
	prism.product_root = COMMON_PRODUCT_ROOT

	setup_driver(prism)
	setup_common_libs(prism)

	return [prism]


def get_libgemini():
	libgemini = Product(name="gemini", output=ProductType.DynamicLibrary)
	libgemini.root = "../"
	libgemini.sources += [
		"src/sdk/**.cpp",
		"src/sdk/**.h"
	]
	libgemini.sources += [
		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c"),

		# include this almagamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp")
	]

	libgemini.defines += [
		"JSON_IS_AMALGAMATION"
	]

	libgemini.includes += [
		"src/sdk",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3"),
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp")		
	]




	macosx = libgemini.layout(platform="macosx")
	macosx.sources = [
		"src/sdk/gemini/platform/osx/osx_gemgl.*",
		"src/sdk/gemini/platform/osx/*.m*",
		"src/sdk/gemini/platform/osx/*.h*"
	]
	macosx.links = [
		"Cocoa.framework",
		"OpenGL.framework",
		"AudioToolbox.framework",
		"OpenAL.framework"
	]	

	#libgemini.defines += [
	#	"GEMINI_USE_SDL2=1"
	#]
	libgemini.product_root = "latest/lib/${CONFIGURATION}_${ARCHITECTURE}"



	return [libgemini]

def arguments(parser):
	parser.add_argument("--with-glesv2", dest="glesv2", action="store_true", help="Build with GLES V2", default=False)
	parser.add_argument("--raspberrypi", dest="raspberrypi", action="store_true", help="Build for the RaspberryPi", default=False)
	parser.add_argument("--indextype", dest="index_type", choices=["uint", "ushort"], type=str, default="uint", help="Set the IndexBuffer type; defaults to uint")
	parser.add_argument("--with-xwl", dest="with_xwl", action="store_true", help="Build with xwl", default=False)
	parser.add_argument("--with-sdl2", dest="with_sdl2", action="store_true", help="Build with SDL2", default=True)

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




	target_platform = kwargs.get("target_platform")

	gemini = Product(name="gemini_desktop", output=ProductType.Application)
	gemini.root = "../"
	gemini.product_root = COMMON_PRODUCT_ROOT
	gemini.object_root = "obj"

	setup_common_libs(gemini)

	gemini.dependencies += [
		Dependency(file="sqrat.py"),
		Dependency(file="squirrel3.py", products=["squirrel", "sqstdlib"]),
		Dependency(file="nom.py"),
		Dependency(file="bullet2.py", products=["BulletSoftBody", "BulletDynamics", "BulletCollision", "LinearMath"]),
		#Dependency(file="box2d.py")

	]

	# common sources
	setup_common_variables(arguments, target_platform, gemini)

	setup_driver(gemini)



	# more sources
	gemini.sources += [
		"src/kernels/**.c*",
		"src/core/*.*",
		"src/core/gldrivers/opengl_common.*",
		"src/core/audio/openal.*",
		"src/core/assets/*.*",
		"src/contrib/*",
		"src/game/**.*",
	]

	gemini.excludes += [
		"src/kernels/test_assimp.cpp",
		"src/kernels/project_huckleberry.cpp"
	]



	

	if target_platform.get() in DESKTOP:
		gemini.sources += [
			"src/core/desktop/kernel_desktop.cpp",
			"src/core/audio/openal_vorbis_decoder.*",
			"src/core/gldrivers/opengl_core32.*"
		]

		gemini.includes += [
			"src/core/audio"
		]

		gemini.prebuild_commands = [
			"python %s -c ../assets/desktop.conf -y" % (BLACKSMITH_PATH)
		]

		xwl_params = []
		if target_platform.get() == "linux":
			xwl_params.append("--with-x11")
			#xwl_params.append("--with-egl")

		if arguments.with_xwl:
			gemini.dependencies += [
				Dependency(file="xwl.py", products=["xwl"], arguments=xwl_params)
			]
			gemini.defines += [
				"GEMINI_USE_XWL=1"
			]
		elif arguments.with_sdl2:
			gemini.dependencies += [
				Dependency(file="sdl2.py")
			]
			gemini.defines += [
				"GEMINI_USE_SDL2=1"
			]

		macosx = gemini.layout(platform="macosx")
		macosx.sources = [
			"src/core/osx/osx_gemgl.*",
			"src/core/osx/*.m*",
			"src/core/osx/*.h*"
		]
		macosx.links = [
			"Cocoa.framework",
			"OpenGL.framework",
			"AudioToolbox.framework",
			"OpenAL.framework"
		]

		macosx.driver.infoplist_file = "resources/osx/Info.plist"
		macosx.resources = [
			"resources/osx/en.lproj/*.xib",
			"resources/osx/en.lproj/*.strings"
		]



		linux = gemini.layout(platform="linux")
		linux.sources += [
			"src/core/desktop/entry.cpp"
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
			linux.sources += [
				"src/core/gldrivers/opengl_glesv2.*"
			]

			linux.defines += [
				"PLATFORM_USE_GLES2=1"
			]			
		else:
			linux.links += ["GL"]









	iphoneos = gemini.layout(platform="iphoneos")
	iphoneos.prebuild_commands += [
		"python %s -c ../assets/ios.conf -y" % (BLACKSMITH_PATH)
	]

	iphoneos.sources += [
		"src/core/audio/audio_extaudio_decoder.*"
	]

	iphoneos.resources = [
		"resources/ios/Settings.bundle",
		"resources/ios/en.lproj/*.*"
	]


	tools = get_tools()

	libgemini = get_libgemini()

	return [gemini] + tools + libgemini


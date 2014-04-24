import os
import logging

from pegasus.models import Product, ProductType, Dependency

DEPENDENCIES_FOLDER = "dependencies"
DESKTOP = ["macosx", "linux", "windows"]
BLACKSMITH_PATH = "../tools/blacksmith/blacksmith.py"

def setup_common_variables(arguments, target_platform, product):
	product.sources = [
		"src/*.c*",
		"src/*.h*",

		# dependencies
		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c"),

		# include this almagamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp"),

		os.path.join(DEPENDENCIES_FOLDER, "font-stash/fontstash.c"),
		os.path.join(DEPENDENCIES_FOLDER, "font-stash/stb_truetype.c"),

		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.c"),
		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.h")
	]


	product.includes = [
		"src",
		"src/game",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3"),
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp"),
		os.path.join(DEPENDENCIES_FOLDER, "font-stash"),
		os.path.join(DEPENDENCIES_FOLDER, "slim")
	]

	# TODO: Allow generic *.DS_Store excludes
	product.excludes = [
		"src/core/.DS_Store",
		"src/core/assets/.DS_Store",
		"src/contrib/.DS_Store",
		"src/game/.DS_Store",
		"src/entry.cpp"
	]

	product.defines = [
		"JSON_IS_AMALGAMATION"
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


def arguments(parser):
	parser.add_argument("--with-glesv2", dest="glesv2", action="store_true", help="Build with GLES V2", default=False)
	parser.add_argument("--raspberrypi", dest="raspberrypi", action="store_true", help="Build for the RaspberryPi", default=False)
	parser.add_argument("--indextype", dest="index_type", choices=["uint", "ushort"], type=str, default="uint", help="Set the IndexBuffer type; defaults to uint")

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform")

	gemini = Product(name="gemini_desktop", output=ProductType.Application)
	gemini.root = "../"
	gemini.product_root = "latest/bin/${CONFIGURATION}_${ARCHITECTURE}"
	gemini.object_root = "obj"


	gemini.dependencies = [
		Dependency(file="glm.py"),
		Dependency(file="sqrat.py"),
		Dependency(file="squirrel3.py", products=["squirrel", "sqstdlib"]),
		Dependency(file="nom.py"),
		Dependency(file="bullet2.py", products=["BulletSoftBody", "BulletDynamics", "BulletCollision", "LinearMath"]),
		#Dependency(file="box2d.py")

	]

	if target_platform.get() in ["macosx", "linux", "windows"]:
		gemini.dependencies += [
			Dependency(file="xwl.py", products=["xwl"])
		]

	# common sources
	setup_common_variables(arguments, target_platform, gemini)

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

	gemini.includes += [
		"src",
		"src/core",
		"src/core/audio",
		"src/contrib"
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
		macosx.driver.macosx_deployment_target = "10.8"
		macosx.driver.sdkroot = "macosx10.9"
		macosx.resources = [
			"resources/osx/en.lproj/*.xib",
			"resources/osx/en.lproj/*.strings"
		]


		linux = gemini.layout(platform="linux")
		linux.sources = [
			"src/core/desktop/entry.cpp"
		]

		linux.links = [
			"pthread",
			"dl",
			"openal"
		]

		linux.linkflags = [
			"-Wl,-rpath,."
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


	return [gemini]


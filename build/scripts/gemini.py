import os
import logging

from pegasus.models import Product, ProductType, Dependency


# GCC 4.3+ needs -std=c++0x
# GCC 4.7+ supports -std=c++11

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

libglm = Dependency(file="glm.py")
librecastnavigation = Dependency(file="recastnavigation.py")
libfreetype = Dependency(file="freetype.py")
rapidjson = Dependency(file="rapidjson.py")

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
		libglm
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

		os.path.join(DEPENDENCIES_FOLDER, "stb"),

		os.path.join(DEPENDENCIES_FOLDER, "murmur3")
	]


	product.dependencies += [
		libglm
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

def setup_driver(arguments, product):

	#macosx = product.layout(platform="macosx")
	#macosx.driver.macosx_deployment_target = "10.8"
	#macosx.driver.sdkroot = "macosx10.9"

	product.excludes += [
		"*.DS_Store"
	]

	if arguments.gles:
		product.defines += [
			"PLATFORM_GLES2_SUPPORT=1"
		]
	elif arguments.opengl:
		product.defines += [
			"PLATFORM_OPENGL_SUPPORT=1"
		]

	gcc_flags = [
		# We need exceptions for jsoncpp.
		#"-fno-exceptions",
		"-fno-rtti",
		"-Wall",
		"-Wpedantic",
		"-Wextra",
		"-Wreorder",
		"-Wunused",
		"-Wsign-compare",

		# enable this for a cleanup pass
		"-Wno-unused-parameter"
	]

	mac_debug = product.layout(platform="macosx", configuration="debug")
	mac_debug.driver.gcc_optimization_level="0"
	mac_debug.driver.debug_information_format="dwarf-with-dsym"

	mac_debug.cflags += gcc_flags + [

		# This can be useful; however some third-party code uses macros
		# where this starts to accumulate fast. I'm looking at you, Bullet.
		"-Wno-extra-semi",

		# disable C++98 compat since we're building with C++11
		"-Wno-c++98-compat",
		"-Wno-c++98-compat-pedantic",

		# this is technically bad, but it's entrenched at present
		"-Wno-gnu-zero-variadic-macro-arguments",

		# print full template backtrace
		"-ftemplate-backtrace-limit=0"
	]

	mac_release = product.layout(platform="macosx", configuration="release")

	linux = product.layout(platform="linux")

	if arguments.raspberrypi:
		linux.defines += [
			"PLATFORM_RASPBERRYPI=1"
		]
	if arguments.with_egl:
		linux.defines += [
			"PLATFORM_EGL_SUPPORT=1"
		]
	if arguments.with_x11:
		linux.defines += [
			"PLATFORM_X11_SUPPORT=1"
		]

		linux.links += [
			"Xinerama"
		]

	linux.cflags += gcc_flags

	linux_debug = product.layout(platform="linux", configuration="debug")
	linux_debug.cflags += [
		"-g"
	]

	linux.links += [
		"pthread",
		"dl"
	]

	if product.output in [ProductType.Application, ProductType.Commandline]:
		# use current directory to resolve shared libraries
		linux.linkflags += [
			"-Wl,-rpath='$$$\ORIGIN'",
		]

		if arguments.raspberrypi:
			linux.libdirs += [
				"/opt/vc/lib"
			]
			linux.links += [
				# Broadcom
				"bcm_host",

				# VideoCore
				"vcos",

				"EGL",
				"GLESv2"
			]

		if arguments.gles:
			linux.links += [
				"GLESv2"
			]
		else:
			linux.links += [
				"GL"
			]

def get_tools(arguments, libui, libruntime, librenderer, libplatform, libcore, **kwargs):
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
	setup_driver(arguments, muse)
	setup_common_tool(muse)
	tools.append(muse)

	#kraken = get_kraken(arguments, libruntime, libplatform, libcore, librenderer, **kwargs)
	#tools.append(kraken)

	orion = get_orion(arguments, libui, libruntime, libplatform, libcore, librenderer, **kwargs)
	tools.append(orion)

	return tools


def get_libcore(arguments, target_platform):
	libcore = Product(name="core", output=ProductType.DynamicLibrary)
	setup_driver(arguments, libcore)
	libcore.project_root = COMMON_PROJECT_ROOT
	libcore.root = "../"
	libcore.sources += [
		"src/core/*.cpp",
		"src/core/*.h",
		"src/core/memory/*.h",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c")
	]


	libcore.includes += [
		"src",
		"src/core",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3")
	]

	libcore.dependencies += [
		Dependency(file="glm.py"),
		rapidjson
	]

	return libcore

def get_libplatform(arguments, target_platform):
	libplatform = Product(name="platform", output=ProductType.DynamicLibrary)
	setup_driver(arguments, libplatform)
	libplatform.project_root = COMMON_PROJECT_ROOT
	libplatform.root = "../"
	libplatform.sources += [
		"src/platform/graphics_provider.cpp",
		"src/platform/graphics_provider.h",
		"src/platform/kernel.cpp",
		"src/platform/kernel.h",
		"src/platform/kernel_events.h",
		"src/platform/platform.cpp",
		"src/platform/platform.h",
		"src/platform/platform_internal.h",
		"src/platform/input.cpp",
		"src/platform/input.h",
		"src/platform/window.cpp",
		"src/platform/window.h",
		"src/platform/window_provider.cpp",
		"src/platform/window_provider.h"
	]

	libplatform.includes += [
		"src/platform"
	]

	macosx = libplatform.layout(platform="macosx")
	macosx.sources += [
		# application
		"src/platform/application/cocoa/cocoa_appdelegate.mm",
		"src/platform/application/cocoa/cocoa_appdelegate.h",
		"src/platform/application/cocoa/cocoa_application.mm",
		"src/platform/application/cocoa/cocoa_application.h",

		# backend
		"src/platform/backend/osx/osx_backend.mm",
		"src/platform/backend/osx/cocoa_common.h",

		# dylib
		"src/platform/dylib/osx/osx_dylib.cpp",
		"src/platform/dylib/posix/posix_dlopen.cpp",

		# filesystem
		"src/platform/filesystem/osx/osx_filesystem.mm",
		"src/platform/filesystem/posix/posix_filesystem_common.cpp",

		# serial
		"src/platform/serial/posix/posix_serial.cpp",

		# thread
		"src/platform/thread/osx/osx_thread.cpp",
		"src/platform/thread/posix/posix_thread_common.cpp",
		"src/platform/thread/posix/posix_thread.h",

		# time
		"src/platform/time/osx/osx_timer.cpp",
		"src/platform/time/posix/posix_datetime.cpp",

		# window
		"src/platform/window/cocoa/cocoa_window_provider.mm",
		"src/platform/window/cocoa/cocoa_window_provider.h",
		"src/platform/window/cocoa/cocoa_openglview.mm",
		"src/platform/window/cocoa/cocoa_openglview.h",
		"src/platform/window/cocoa/cocoa_window.mm",
		"src/platform/window/cocoa/cocoa_window.h"
	]

	macosx.includes += [
		"src/platform/posix",
		"src/platform/backend/osx",
		"src/platform/window/cocoa"
	]

	macosx.links += [
		"Cocoa.framework",
		"IOKit.framework"
	]

	macosx.defines += [
		"PLATFORM_OPENGL_SUPPORT=1"
	]


	linux = libplatform.layout(platform="linux")
	linux.sources += [
		# backend
		"src/platform/backend/linux/linux_backend.cpp",
		"src/platform/backend/linux/linux_backend.h",

		# dylib
		"src/platform/dylib/posix/posix_dylib.cpp",
		"src/platform/dylib/posix/posix_dlopen.cpp",

		# filesystem
		"src/platform/filesystem/posix/posix_filesystem.cpp",
		"src/platform/filesystem/posix/posix_filesystem_common.cpp",

		# serial
		"src/platform/serial/posix/posix_serial.cpp",

		# thread
		"src/platform/thread/posix/posix_thread.cpp",
		"src/platform/thread/posix/posix_thread_common.cpp",
		"src/platform/thread/posix/posix_thread.h",

		# time
		"src/platform/time/posix/posix_datetime.cpp",
		"src/platform/time/posix/posix_timer.cpp",
	]


	linux.includes += [
		"src/platform/backend/linux"
	]

	#
	# x11 support
	if arguments.with_x11:
		linux.sources += [
			"src/platform/window/x11/*.cpp",
			"src/platform/window/x11/*.h"
		]
		linux.sources += [
			"src/platform/graphics/x11/*.cpp",
			"src/platform/graphics/x11/*.h"
		]

	#
	# egl support
	if arguments.with_egl:
		linux.sources += [
			"src/platform/graphics/egl/*.cpp",
			"src/platform/graphics/egl/*.h"
		]

	#
	# Raspberry Pi: Add support for DispmanX, VideoCore
	if arguments.raspberrypi:
		linux.sources += [
			"src/platform/window/dispmanx/*.cpp",
			"src/platform/window/dispmanx/*.h"
		]
		linux.includes += [
			"/opt/vc/include",
			"/opt/vc/include/interface/vcos/pthreads",
			"/opt/vc/include/interface/vmcs_host/linux"
		]


	windows = libplatform.layout(platform="windows")
	windows.sources += [
		# backend
		"src/platform/backend/windows/win32_backend.cpp",

		# dylib
		"src/platform/dylib/windows/win32_dylib.cpp",

		# filesystem
		"src/platform/filesystem/windows/win32_filesystem.cpp",

		# serial
		"src/platform/serial/win32/win32_serial.cpp",

		# thread
		"src/platform/thread/windows/win32_thread.cpp",
		"src/platform/thread/windows/windows_thread.h",

		# time
		"src/platform/time/windows/win32_time.cpp",

		# window
		"src/platform/window/win32/win32_window.cpp",
		"src/platform/window/win32/win32_window.h",
		"src/platform/window/win32/win32_window_provider.cpp",
		"src/platform/window/win32/win32_window_provider.h"
	]

	windows.links += [
		"Shlwapi",
		"user32"
	]

	return libplatform



def get_librenderer(arguments, target_platform, libruntime):
	librenderer = Product(name="renderer", output=ProductType.DynamicLibrary)
	setup_driver(arguments, librenderer)
	librenderer.project_root = COMMON_PROJECT_ROOT
	librenderer.root = "../"
	librenderer.sources += [
		"src/renderer/*.*",

		os.path.join(DEPENDENCIES_FOLDER, "stb", "stb_image.h"),
		os.path.join(DEPENDENCIES_FOLDER, "stb", "stb_truetype.h"),

		# include this amalgamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp")
	]

	librenderer.defines += [
		"JSON_IS_AMALGAMATION"
	]

	librenderer.includes += [
		"src/renderer",
		"src/renderer/gl",

		os.path.join(DEPENDENCIES_FOLDER, "stb"),
		os.path.join(DEPENDENCIES_FOLDER, "fontstash/src"),
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp")
	]

	librenderer.dependencies += [
		libruntime,
		libfreetype
	]

	if arguments.opengl:
		librenderer.sources += [
			"src/renderer/gl/*.cpp",
			"src/renderer/gl/*.h",
			"src/renderer/gl/opengl/*.cpp",
			"src/renderer/gl/opengl/*.h"
		]
		librenderer.excludes += [
			"src/renderer/gl/opengl/opengl_21.*",
		]
	elif arguments.gles:
		librenderer.sources += [
			"src/renderer/gl/*.cpp",
			"src/renderer/gl/*.h",
			# "src/renderer/gl/gles2/*.cpp",
			# "src/renderer/gl/gles2/*.h"
			"src/renderer/gl/gles2/*.cpp",
			"src/renderer/gl/gles2/*.h"
		]
		librenderer.excludes += [
			"src/renderer/gl/gles2/opengl_glesv2.*"
		]
	macosx = librenderer.layout(platform="macosx")
	if arguments.opengl:
		macosx.sources += [
			"src/renderer/gl/gemgl_osx.mm"
		]

		macosx.links += [
			"Cocoa.framework",
			"OpenGL.framework"
		]

	return librenderer

def get_libruntime(arguments, target_platform, libui):
	libruntime = Product(name="runtime", output=ProductType.DynamicLibrary)
	setup_driver(arguments, libruntime)
	libruntime.project_root = COMMON_PROJECT_ROOT
	libruntime.root = "../"
	libruntime.sources += [
		"src/runtime/*.c",
		"src/runtime/*.cpp",
		"src/runtime/*.h",

		# include this amalgamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp")
	]

	libruntime.defines += [
		"JSON_IS_AMALGAMATION"
	]

	libruntime.includes += [
		"src/runtime",
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp"),
	]

	libruntime.dependencies.append(libui)

	return libruntime

def get_sdk(arguments, links, **kwargs):
	global_params = kwargs.get("global_params")
	target_platform = kwargs.get("target_platform")

	sdk = Product(name="sdk", output=ProductType.DynamicLibrary)
	sdk.project_root = COMMON_PROJECT_ROOT
	sdk.root = "../"
	sdk.sources += [
		"src/sdk/audio.cpp",
		"src/sdk/debugdraw.cpp",
		"src/sdk/engine.cpp",
		"src/sdk/physics.cpp"
	]
	sdk.includes += [
		"src/sdk/include"
	]

	setup_driver(arguments, sdk)
	setup_common_tool(sdk)

	sdk.dependencies.extend(links)

	return sdk

def get_libui(arguments, target_platform, libcore):
	ui = Product(name="ui", output=ProductType.DynamicLibrary)
	setup_driver(arguments, ui)
	ui.root = "../"
	ui.project_root = COMMON_PROJECT_ROOT


	# for maths
	ui.dependencies += [
		libcore,
		libglm
	]

	ui.sources = [
		"src/ui/**.c*",
		"src/ui/**.h"
	]

	ui.includes = [
		"src"
	]

	debug = ui.layout(configuration="debug")
	debug.defines = [
		"DEBUG"
	]
	release = ui.layout(configuration="release")


	#
	# macosx
	#
	macosx_debug = ui.layout(platform="macosx", configuration="debug")
	macosx_debug.driver.gcc_optimization_level = "0"
	macosx_debug.driver.gcc_generate_debugging_symbols = "YES"
	macosx_debug.driver.debug_information_format = "dwarf-with-dsym"

	macosx_release = ui.layout(platform="macosx", configuration="release")
	macosx_release.driver.gcc_optimization_level = "2"
	macosx_release.driver.gcc_generate_debugging_symbols = "NO"

	#
	# linux
	#
	linux_debug = ui.layout(configuration="debug", platform="linux")
	linux_debug.cflags = [
		"-g",
		"-Wall",
		"-O0"
	]

	linux_release = ui.layout(configuration="release", platform="linux")
	linux_release.cflags = [
		"-O2"
	]

	windows = ui.layout(platform="windows")
	windows.defines += [
		"UNICODE"
	]

	return ui

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

	setup_driver(arguments, rnd)
	setup_common_tool(rnd)

	rnd.dependencies.extend(links)

	rnd_macosx = rnd.layout(platform="macosx")
	rnd_macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	rnd_linux = rnd.layout(platform="linux")
	rnd_linux.links += [
		"GL",
		"udev"
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

def create_unit_test(arguments, name, dependencies, source, output_type = ProductType.Commandline):
	product = Product(name=name, output=output_type)
	product.project_root = COMMON_PROJECT_ROOT
	product.root = "../"
	product.includes += [
		"tests"
	]
	product.sources += [
		"tests/unit_test.h",
		source
	]

	if output_type == ProductType.Application:
		macosx = product.layout(platform="macosx")
		base_path = "tests/src/%s" % name
		resource_path = "tests/resources"
		macosx.driver.infoplist_file = "../%s/osx/Info.plist" % base_path
		macosx.resources = [
			"%s/osx/en.lproj/*.xib" % base_path,
			"%s/osx/en.lproj/*.strings" % base_path,
			"%s/osx/media.xcassets/" % base_path,
			"%s/fonts/" % resource_path,
			"%s/shaders/" % resource_path,
			"%s/textures/" % resource_path
		]

	product.dependencies.extend(dependencies)

	setup_driver(arguments, product)

	return product

def get_unit_tests(arguments, libcore, libplatform, librenderer, libruntime, libglm, libui, **kwargs):
	return [
		create_unit_test(arguments, "test_core", [rapidjson, libcore, libglm], "tests/src/test_core.cpp"),
		create_unit_test(arguments, "test_platform", [libplatform, libcore, libglm], "tests/src/test_platform.cpp"),
		create_unit_test(arguments, "test_runtime", [rapidjson, libruntime, libui, libplatform, libcore, libglm], "tests/src/test_runtime.cpp"),
		create_unit_test(arguments, "test_render", [rapidjson, libfreetype, librenderer, libruntime, libui, libplatform, libcore, libglm], "tests/src/test_render.cpp", ProductType.Application),
		create_unit_test(arguments, "test_ui", [rapidjson, libfreetype, librenderer, libruntime, libui, libplatform, libcore, libglm], "tests/src/test_ui.cpp", ProductType.Application)
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

	setup_driver(arguments, kraken)
	setup_common_tool(kraken)

	kraken.dependencies.extend([
		libplatform,
		libcore,
		librenderer,
		libui
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

def get_orion(arguments, libui, libruntime, libplatform, libcore, librenderer, **kwargs):
	orion = Product(name="orion", output=ProductType.Application)
	orion.project_root = COMMON_PROJECT_ROOT
	orion.product_root = COMMON_PRODUCT_ROOT
	orion.root = "../"

	setup_driver(arguments, orion)
	setup_common_tool(orion)

	orion.dependencies.extend([
		libfreetype,
		libui,
		libruntime,
		librenderer,
		libplatform,
		libcore
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

	return orion

def arguments(parser):
	parser.add_argument("--with-gles", dest="gles", action="store_true", help="Build with GLES support", default=False)
	parser.add_argument("--raspberrypi", dest="raspberrypi", action="store_true", help="Build for the RaspberryPi; implies EGL + OpenGLES", default=False)
	parser.add_argument("--with-opengl", dest="opengl", action="store_true", help="Build with support for full OpenGL; mutually exclusive with OpenGL ES", default=True)

	parser.add_argument("--with-civet", dest="with_civet", action="store_true", help="Build with CivetServer", default=True)
	parser.add_argument("--no-civet", dest="with_civet", action="store_false", help="Build without CivetServer")

	parser.add_argument("--with-x11", dest="with_x11", action="store_true", help="Build with X11 support", default=False)
	parser.add_argument("--with-egl", dest="with_egl", action="store_true", help="Build with EGL support", default=False)

	parser.add_argument("--with-tools", dest="with_tools", action="store_true", help="Build with support for tools", default=False)
	parser.add_argument("--with-tests", dest="with_tests", action="store_true", help="Build with support for unit tests", default=False)

def products(arguments, **kwargs):

	# warn about SDKs...
	if arguments.with_tools:
		logging.warn("Generating projects for products which require private SDKs...")
		logging.warn("Please be sure you've run build/scripts/sync_sdks.py!")
		if not os.path.exists(os.path.join("build", SDKS_FOLDER)):
			raise Exception("SDKs folder is missing!")

	# global params will be inherited by all dependent products
	global_params = kwargs.get("global_params")

	#global_linux = global_params.layout(platform="linux")
	#global_linux.driver.requires_stdlib="4.9.0"

	g_macosx = global_params.layout(platform="macosx")

	# use C++11
	g_macosx.driver.clang_cxx_language_standard = "c++0x"

	# use LLVM C++ standard library with C++11 support
	g_macosx.driver.clang_cxx_library = "libc++"
	g_macosx.driver.macosx_deployment_target = "10.8"
	g_macosx.driver.sdkroot = ""

	g_macosx.driver.clang_enable_objc_arc = "YES"

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

	if arguments.raspberrypi:
		if not arguments.gles:
			arguments.gles = True

		if not arguments.with_egl:
			arguments.with_egl = True

	if not arguments.gles:
		arguments.opengl = True
	elif arguments.gles:
		arguments.opengl = False
	else:
		raise Exception("Unknown renderer!")



	libcore = get_libcore(arguments, target_platform)

	libplatform = get_libplatform(arguments, target_platform)
	libplatform.dependencies += [libcore]

	libui = get_libui(arguments, target_platform, libcore)

	libruntime = get_libruntime(arguments, target_platform, libui)
	libruntime.dependencies += [libcore, libplatform, Dependency(file="glm.py")]

	librenderer = get_librenderer(arguments, target_platform, libruntime)
	librenderer.dependencies += [libui, libcore, libplatform, Dependency(file="glm.py")]

	# don't add this until we clean up the shaderconfig dependency on libruntime
	#libruntime.dependencies.append(librenderer)



	tools = []
	if arguments.with_tools:
		tools = get_tools(arguments, libui, libruntime, librenderer, libplatform, libcore, **kwargs)
	else:
		logging.warn("Compiling WITHOUT tools...")

	gemini = Product(name="gemini_desktop", output=ProductType.Application)
	gemini.project_root = COMMON_PROJECT_ROOT
	gemini.root = "../"
	gemini.product_root = COMMON_PRODUCT_ROOT

	setup_common_libs(arguments, gemini)

	gemini.sources += [
		"src/engine/gemini.cpp"
	]

	gemini.dependencies += [
		libruntime,
		librenderer,
		libfreetype,
		libplatform
	]

	gemini.dependencies += [
		libcore,
		Dependency(file="sqrat.py"),
		#Dependency(file="squirrel3.py", products=["squirrel", "sqstdlib"]),
		libui,
		Dependency(file="bullet2.py", products=["BulletSoftBody", "BulletDynamics", "BulletCollision", "LinearMath"]),
		librecastnavigation
	]

	# common sources
	setup_common_variables(arguments, target_platform, gemini)

	setup_driver(arguments, gemini)

	# more sources
	gemini.sources += [
		"src/engine/*.*",
		"src/engine/audio/openal.*",
		"src/engine/assets/*.*",
		"src/engine/game/**.*",

		os.path.join(DEPENDENCIES_FOLDER, "stb", "stb_image.h"),
		os.path.join(DEPENDENCIES_FOLDER, "stb", "stb_freetype.h"),
		os.path.join(DEPENDENCIES_FOLDER, "stb", "stb_vorbis.c")
	]

	gemini.excludes += [
		"*.DS_Store",
		"src/engine/kernels/test_assimp.cpp",
		"src/engine/kernels/project_huckleberry.cpp",

		"src/engine/kernels/test_bullet2.cpp",
		"src/engine/kernels/test_mobile.cpp",
		"src/engine/kernels/test_ui.cpp"
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

		if arguments.with_x11:
			linux.links += ["X11"]

		if arguments.with_egl:
			linux.links += ["EGL"]

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

	libsdk = get_sdk(arguments, [libruntime, librenderer, libplatform, libcore], **kwargs)
	gemini.dependencies.append(libsdk)

	rnd = get_rnd(arguments, [libruntime, librenderer, libplatform, libcore], **kwargs)
	tests = []
	if arguments.with_tests:
		# Ugh, for now, we also link in libui because the runtime requires it.
		# I feel like pegasus should identify such dependencies and take care of it in the future.
		# Though, for now, just link it in.
		tests = get_unit_tests(arguments, libcore, libplatform, librenderer, libruntime, Dependency(file="glm.py"), libui, **kwargs)

	return [libui, librenderer, libruntime, libplatform, libcore] + [libsdk, gemini] + tools + [rnd] + tests


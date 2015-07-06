import logging
from pegasus.models import Product, ProductType


def arguments(parser):
	parser.add_argument("--with-egl", dest="with_egl", action="store_true", help="Compile with EGL support", default=None)
	parser.add_argument("--with-x11", dest="with_x11", action="store_true", help="Compile with X11 support", default=None)
	parser.add_argument("--with-rpi", dest="with_rpi", action="store_true", help="Compile with RaspberryPi support", default=None)

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)

	xwl = Product(name="xwl", output=ProductType.DynamicLibrary)
	xwl.root = "../dependencies/xwl"
	xwl.project_root = "_projects"


	xwl.sources = [
		"src/*.c",
		"src/**.h",
		"include/**.h"
	]

	xwl.excludes = [
		"samples/**.c",
		"src/ogl.c"
	]

	xwl.includes = [
		"include"
	]

	xwl.links = []
	xwl.cflags = []

	debug = xwl.layout(configuration="debug")
	debug.defines = [
		"DEBUG"
	]
	release = xwl.layout(configuration="release")


	#
	# macosx
	#
	macosx = xwl.layout(platform="macosx")
	macosx.sources += [
		"src/**.m"
	]

	macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	macosx_debug = xwl.layout(platform="macosx", configuration="debug")
	macosx_debug.driver.gcc_optimization_level = "0"
	macosx_debug.driver.gcc_generate_debugging_symbols = "YES"
	macosx_debug.driver.debug_information_format = "dwarf-with-dsym"

	macosx_release = xwl.layout(platform="macosx", configuration="release")
	macosx_release.driver.gcc_optimization_level = "2"
	macosx_release.driver.gcc_generate_debugging_symbols = "NO"

	#
	# linux
	#
	linux = xwl.layout(platform="linux")

	linux_debug = xwl.layout(configuration="debug", platform="linux")
	linux_debug.cflags = [
		"-g",
		"-Wall",
		"-O0"
	]

	linux_release = xwl.layout(configuration="release", platform="linux")
	linux_release.cflags = [
		"-O2",
		"-fPIC"
	]

	if arguments.with_x11:
		linux.defines += [
			"XWL_WITH_X11=1"
		]

		linux.links += [
			"Xinerama",
			"X11"
		]

		linux.sources += [
			"src/platforms/x11/**.c",
			"include/xwl/platforms/x11/**.h"
		]


	if arguments.with_rpi:
		linux.defines += [
			"RASPBERRYPI=1"
		]

		linux.sources += [
			"src/platforms/egl/**.c",
			"include/xwl/platforms/egl/**.h",
			"src/platforms/rpi/**.c",
			"include/xwl/platforms/rpi/**.h",	
		]

		linux.includes += [
			"/opt/vc/include",
			"include/xwl/platforms/rpi"
		]

		linux.libdirs += [
			"/opt/vc/lib"
		]

		linux.cflags += [
			"-Wall"
		]

		linux.links += [
			"GLESv2"
		]
	else:
		linux.links += [
			"GL",
			"dl"
		]

	if arguments.with_egl:
		linux.defines += [
			"XWL_WITH_EGL=1"
		]
		
		linux.sources += [
			"src/platforms/egl/**.c",
			"include/xwl/platforms/egl/**.h"
		]

		linux.includes += [
			"include/xwl/platforms/egl"
		]

		linux.links += [
			"EGL"
		]

	windows = xwl.layout(platform="windows")
	windows.defines += [
		"XWL_DLL=1",
		"UNICODE"
	]

	windows.sources += [
		"src/platforms/win32/**.c",
		"include/xwl/platforms/win32/**.h"
	]

	windows.links += [
		"opengl32"
	]	

	return [xwl]
import logging
from pegasus.models import Product, ProductType


def arguments(parser):
	pass

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)

	nom = Product(name="nom", output=ProductType.StaticLibrary)
	nom.root = "../dependencies/nom"
	nom.project_root = "_projects"
	nom.object_root = "${PROJECT_ROOT}/obj"


	nom.sources = [
		"src/**.c*",
		"src/**.h",
		"include/**.h*"
	]

	nom.excludes = [
		"src/samples/*"
	]

	nom.includes = [
		"include"
	]

	debug = nom.layout(configuration="debug")
	debug.defines = [
		"DEBUG"
	]
	release = nom.layout(configuration="release")


	#
	# macosx
	#
	macosx = nom.layout(platform="macosx")
	macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework"
	]

	macosx_debug = nom.layout(platform="macosx", configuration="debug")
	macosx_debug.driver.gcc_optimization_level = "0"
	macosx_debug.driver.gcc_generate_debugging_symbols = "YES"
	macosx_debug.driver.debug_information_format = "dwarf-with-dsym"

	macosx_release = nom.layout(platform="macosx", configuration="release")
	macosx_release.driver.gcc_optimization_level = "2"
	macosx_release.driver.gcc_generate_debugging_symbols = "NO"

	#
	# linux
	#
	linux = nom.layout(platform="linux")
	linux.links = [
		"X11",
		"GL"
	]

	linux_debug = nom.layout(configuration="debug", platform="linux")
	linux_debug.cflags = [
		"-g",
		"-Wall",
		"-O0"
	]

	linux_release = nom.layout(configuration="release", platform="linux")
	linux_release.cflags = [
		"-O2",
		"-fvisibility=hidden",
		"-fPIC"
	]

	windows = nom.layout(platform="windows")
	windows.defines += [
		"UNICODE"
	]

	windows.links = [
		"opengl32"
	]

	return [nom]

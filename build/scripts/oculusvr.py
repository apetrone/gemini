import os
import logging
from pegasus.models import Product, ProductType


SDK_ROOT = "../dependencies/sdks"

def arguments(parser):
	pass

def products(arguments, **kwargs):

	LIBOVR_VERSION = "0.4.3"

	target_platform = kwargs.get("target_platform", None)

	# assemble these mappings for oculus sdk
	to_oculus_platform = {
		"macosx" : "Mac",
		"linux" : "Linux",
		"windows": "Win32"
	}

	to_oculus_arch = {
		"macosx": "",
		"linux": "/x86_64",
		"windows": ""
	}

	# TODO: customize these per-platform
	vars = {
		"platform": to_oculus_platform[target_platform.name],
		"title_configuration": "Debug",
		"project" : "Xcode",
		"architecture": to_oculus_arch[target_platform.name]
	}

	ovr = Product(name="ovr", output=ProductType.StaticLibrary)


	# 0.4.1 needs "%(project)s/" between platform and configuration
	# 0.4.3 has "experimental" linux support

	ovr.root = os.path.join(SDK_ROOT, "oculussdk_%s" % LIBOVR_VERSION)

	if target_platform.matches("windows"):
		# need to use its own folder due to line endings
		# until I move that to a better solution
		#ovr.root = os.path.join(SDK_ROOT, "oculussdk_%s_windows" % LIBOVR_VERSION)
		ovr.name = "libovr64d" # use libovr/d or libovr64/d
		ovr.product_root = "LibOVR/Lib/x64/VS2013" # use Win32 or x64 for platform
	else:
		
		ovr.product_root = "LibOVR/Lib/%(platform)s/%(title_configuration)s%(architecture)s" % vars



	ovr.includes = [
		"LibOVR/Include",
		"LibOVR/Src"
	]

	#
	# macosx
	#
	macosx = ovr.layout(platform="macosx")
	# macosx.sources += [
	# 	"src/**.m"
	# ]

	macosx.links += [
		"Cocoa.framework",
		"OpenGL.framework",
		"IOKit.framework",
		#"AudioToolbox.framework"
	]

	# macosx_debug = ovr.layout(platform="macosx", configuration="debug")
	# macosx_debug.driver.gcc_optimization_level = "0"
	# macosx_debug.driver.gcc_generate_debugging_symbols = "YES"
	# macosx_debug.driver.debug_information_format = "dwarf-with-dsym"

	# macosx_release = ovr.layout(platform="macosx", configuration="release")
	# macosx_release.driver.gcc_optimization_level = "2"
	# macosx_release.driver.gcc_generate_debugging_symbols = "NO"


	linux = ovr.layout(platform="linux")
	linux.links = [
		"X11",
		"Xrandr",
		"rt"
	]

	return [ovr]
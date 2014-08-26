import logging
from pegasus.models import Product, ProductType


def arguments(parser):
	pass

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)

	# assemble these mappings for oculus sdk
	to_oculus_platform = {
		"macosx" : "Mac",
		"linux" : "Linux",
		"windows": "Win32"
	}

	vars = {
		"platform": to_oculus_platform[target_platform.name],
		"title_configuration": "Debug"
	}

	ovr = Product(name="ovr", output=ProductType.StaticLibrary)
	ovr.root = "../dependencies/oculussdk"
	ovr.product_root = "LibOVR/Lib/%(platform)s/%(title_configuration)s" % vars

	ovr.includes = [
		"LibOVR/Include"
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


	return [ovr]
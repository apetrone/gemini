import logging

from pegasus.models import Product, ProductType

# products: box2d, HelloWorld, freeglut, glui, Testbed
def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	product = Product(name="box2d", output=ProductType.StaticLibrary)
	product.root = "../dependencies/box2d"
	product.includes = [
		"."
	]

	product.sources = [
		"Box2D/Collision/**.c*",
		"Box2D/Collision/**.h",
		"Box2D/Common/**.c*",
		"Box2D/Common/**.h",

		"Box2D/Dynamics/**.c*",
		"Box2D/Dynamics/**.h",

		"Box2D/Rope/**.c*",
		"Box2D/Rope/**.h"
	]

	debug = product.layout(configuration="debug")
	debug.defines = [
		"DEBUG"
	]

	release = product.layout(configuration="release")

	# setup mac build
	mac_debug = product.layout(configuration="debug", platform="macosx")
	mac_debug.driver.gcc_generate_debugging_symbols = "YES"
	mac_debug.driver.debug_information_format = "dwarf-with-dsym"
	mac_debug.driver.gcc_optimization_level = "0"

	mac_release = product.layout(configuration="release", platform="macosx")
	mac_release.driver.gcc_generate_debugging_symbols = "NO"
	mac_release.driver.gcc_enable_cpp_rtti = "NO"
	mac_release.driver.gcc_strict_aliasing = "NO"
	mac_release.driver.gcc_optimization_level = "2"

	# setup linux build
	linux_debug = product.layout(configuration="debug", platform="linux")
	linux_debug.cflags = ["-Wall", "-g", "-O0"]

	linux_release = product.layout(configuration="release", platform="linux")
	linux_release.cflags = ["-O2"]
	linux_release.linkflags = ["-s"]	

	return [product]


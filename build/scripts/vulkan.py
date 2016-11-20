import logging

from pegasus.models import Product, ProductType

def arguments(parser):
	#parser.add_argument("--squirrel3-path", dest="squirrel3_path", help="The squirrel3 root path", default="../dependencies/squirrel3")
	pass

def add_settings_to_product(target_platform, product):
	product.project_root = "_projects"


	#
	# macosx
	#
	macosx = product.layout(platform="macosx")
	macosx.driver.gcc_enable_cpp_exceptions = "NO"
	macosx.driver.gcc_enable_cpp_rtti = "NO"
	macosx.driver.gcc_strict_aliasing = "NO"
	macosx.driver.gcc_optimization_level = "s"

	mac_debug = product.layout(configuration="debug", platform="macosx")
	mac_debug.driver.gcc_generate_debugging_symbols = "YES"
	mac_debug.driver.debug_information_format = "dwarf-with-dsym"

	mac_release = product.layout(configuration="release", platform="macosx")
	mac_release.driver.gcc_generate_debugging_symbols = "NO"

	#
	# linux
	#
	linux = product.layout(platform="linux")
	linux.cxxflags += [
		"-Wall",
		"-O2"
	]
	linux.linkflags += [
		"-fno-strict-aliasing"
	]

	#
	# windows
	#
	windows = product.layout(platform="windows")
	windows.driver.enhanced_instruction_set = "true"
	windows.driver.floating_point_model = "fast"
	windows.driver.enable_large_addresses = "true"
	windows.driver.characterset = "MultiByte"


def create_vulkan(arguments, **kwargs):
	host_platform = kwargs.get("host_platform", None)
	target_platform = kwargs.get("target_platform", None)

	vulkan = Product(name="vulkan-1", output=ProductType.DynamicLibrary)
	vulkan.root = "../dependencies/vulkan-1.0.30.0"

	vulkan.sources = []

	vulkan.includes = [
		"include"
	]

	# add_settings_to_product(target_platform, squirrel)

	return vulkan

def products(arguments, **kwargs):
	vulkan = create_vulkan(arguments, **kwargs)
	return [vulkan]


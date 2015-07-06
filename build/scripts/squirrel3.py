import logging

from pegasus.models import Product, ProductType

def arguments(parser):
	parser.add_argument("--squirrel3-path", dest="squirrel3_path", help="The squirrel3 root path", default="../dependencies/squirrel3")

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

def create_sqstdlib(arguments, **kwargs):
	# fetch vars
	host_platform = kwargs.get("host_platform", None)
	target_platform = kwargs.get("target_platform", None)

	sqstdlib = Product(name="sqstdlib", output=ProductType.StaticLibrary)
	sqstdlib.root = arguments.squirrel3_path
	sqstdlib.sources = [
		"sqstdlib/*.cpp",
		"sqstdlib/*.h"
	]

	sqstdlib.includes = [
		"include",
		"sqstdlib"
	]

	add_settings_to_product(target_platform, sqstdlib)

	return sqstdlib

def create_squirrel(arguments, **kwargs):
	host_platform = kwargs.get("host_platform", None)
	target_platform = kwargs.get("target_platform", None)

	squirrel = Product(name="squirrel", output=ProductType.StaticLibrary)
	squirrel.root = arguments.squirrel3_path

	squirrel.sources = [
		"squirrel/*.cpp",
		"squirrel/*.h"
	]

	squirrel.includes = [
		"include",
		"squirrel"
	]

	add_settings_to_product(target_platform, squirrel)

	return squirrel

def create_sq(arguments, deps, **kwargs):
	host_platform = kwargs.get("host_platform", None)
	target_platform = kwargs.get("target_platform", None)

	sq = Product(name="sq", output=ProductType.Commandline)
	sq.root = arguments.squirrel3_path
	sq.dependencies = deps

	sq.sources = [
		"sq/sq.cpp"
	]
	sq.includes = [
		"include"
	]

	x86_64 = sq.layout(architecture="x86_64")
	x86_64.defines = [
		"_SQ64"
	]

	add_settings_to_product(target_platform, sq)

	return sq

def products(arguments, **kwargs):
	sqstdlib = create_sqstdlib(arguments, **kwargs)
	squirrel = create_squirrel(arguments, **kwargs)
	sq = create_sq(arguments, [sqstdlib, squirrel], **kwargs)

	return [sqstdlib, squirrel, sq]


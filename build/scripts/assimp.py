import logging

from pegasus.models import Product, ProductType

# July 2014
# It looks like the packaged zlib doesn't quite compile (unknown type z_const)
# So I'm just going to find the system path.

ASSIMP_VERSION = "3.1.1"
ASSIMP_SOVERSION = 3

def find_external_libs(product, target_platform):
	logging.info("TODO: search for zlib")

	zlib = target_platform.find_library("z")
	if zlib:
		product.links += [
			"z"
		]
		product.defines += [
			"ASSIMP_BUILD_NO_OWN_ZLIB"
		]		
	else:
		logging.info("TODO: Couldn't find system zlib. hook up contrib/zlib")
		#product.sources += [
		#	"contrib/zlib/*.c",
		#	"contrib/zlib/*.h"
		#]
		#product.includes += [
		#	"contrib/zlib"
		#]
		pass	

	logging.info("TODO: search for unzip/minizip")

def get_contrib(arguments):
	contrib_sources = [
		"contrib/clipper/*.cpp",
		"contrib/clipper/*.h*",
		"contrib/ConvertUTF/*.c",
		"contrib/ConvertUTF/*.h",
		"contrib/irrXML/*.cpp",
		"contrib/irrXML/*.h",
		"contrib/poly2tri/**.cc",
		"contrib/poly2tri/**.h",
		"contrib/unzip/*.c",
		"contrib/unzip/*.h"
	]

	contrib_includes = [
		"contrib/clipper",
		"contrib/ConvertUTF",
		"contrib/irrXML",
		"contrib/poly2tri",
		"contrib/unzip"
	]

	return (contrib_sources, contrib_includes)

def configure_driver(arguments, product, target_platform):
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
	mac_release.driver.gcc_strict_aliasing = "NO"
	mac_release.driver.gcc_optimization_level = "2"

	# setup linux build
	linux_debug = product.layout(configuration="debug", platform="linux")
	linux_debug.cflags = ["-Wall", "-g", "-O0"]

	linux_release = product.layout(configuration="release", platform="linux")
	linux_release.cflags = ["-O2"]

	# remove all symbol table and relocation information from the executable
	#linux_release.linkflags = ["-s"]

	# setup windows build
	windows_debug = product.layout(configuration="debug", platform="windows")
	windows_release = product.layout(configuration="release", platform="windows")

	logging.info("TODO: setup windows driver settings")

def arguments(parser):
	parser.add_argument("--enable-static", 		dest="staticlib", 			action="store_true", default=False, 	help="Build a static (.a) version of the library")
	parser.add_argument("--boost-workaround", 	dest="boost_workaround", 	action="store_true", default=True, 		help="Build a non-boost version of Assimp")
	parser.add_argument("--enable-export", 		dest="enable_export", 		action="store_true", default=False, 	help="Enable Assimp's export functionality")
	parser.add_argument("--enable-tools", 		dest="enable_tools", 		action="store_true", default=False, 	help="Build supplementary tools for assimp")
	parser.add_argument("--enable-samples", 	dest="enable_samples", 		action="store_true", default=False, 	help="Build samples for assimp")
	parser.add_argument("--enable-tests", 		dest="enable_tests", 		action="store_true", default=False, 	help="Build tests for assimp")	

def products(arguments, **kwargs):

	logging.info("TODO: if compiler is GNUC or GNUCXX and not MINGW")
	logging.info("TODO: \tadd -fPIC")
	logging.info("TODO: \tadd -fvisibility=hidden -Wall")
	logging.info("TODO: else if MSVC")
	logging.info("TODO: add /MP")


	target_platform = kwargs.get("target_platform", None)
	library_output_type = ProductType.StaticLibrary if arguments.staticlib else ProductType.DynamicLibrary
	product = Product(name="assimp", output=library_output_type)
	product.root = "../dependencies/assimp"


	contrib_sources, contrib_includes = get_contrib(arguments)

	product.sources += [
		"code/*.cpp",
		"code/*.h"
	]
	product.sources += contrib_sources

	product.includes = [
		"include",
		"code"
	]

	product.includes += contrib_includes

	if arguments.boost_workaround:
		product.includes += [
			"code/BoostWorkaround"
		]
		product.defines += [
			"ASSIMP_BUILD_BOOST_WORKAROUND"
		]
	else:
		logging.info("TODO: handle compiling assimp with boost.")

	if arguments.enable_export:
		pass
	else:
		product.defines += [
			"ASSIMP_BUILD_NO_EXPORT"
		]
		logging.info("Building an import-only version of Assimp.")


	find_external_libs(product, target_platform)
	#config_vars = {}
	#product.create_config()


	if arguments.enable_tools:
		logging.info("TODO: configure assimp tools")

	if arguments.enable_samples:
		logging.info("TODO build assimp samples")

	if target_platform.matches("windows"):
		logging.info("TODO: install msvc pdb")


	if target_platform.matches("linux"):
		logging.info("TODO: figure out what this should do: ")
		logging.info("\t-DCMAKE_INSTALL_RPATH=. -DCMAKE_BUILD_WITH_INSTALL_RPATH=True")




	# generate revision.h
	revision_vars = {"GIT_COMMIT_HASH": "0856ff9", "GIT_BRANCH": "master"}
	product.create_config("revision.h", template="revision.h.in", variables=revision_vars)

	product.includes += [
		"."
	]

	configure_driver(arguments, product, target_platform)

	return [product]


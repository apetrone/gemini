import logging

from pegasus.models import Product, ProductType

# July 2014
# It looks like the packaged zlib doesn't quite compile (unknown type z_const)
# So I'm just going to find the system path.

ASSIMP_VERSION = "3.1.1"
ASSIMP_SOVERSION = 3

def find_external_libs(product, target_platform):
	logging.info("searching for zlib...")

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


def set_importer_flags(arguments, product):
	""" 
		This method will determine which arguments were passed to the script
		and disable the corresponding importer feature.
	"""
	data = {
		"no-x-importer" : "ASSIMP_BUILD_NO_X_IMPORTER",
		"no-3ds-importer" : "ASSIMP_BUILD_NO_3DS_IMPORTER",
		"no-md3-importer" : "ASSIMP_BUILD_NO_MD3_IMPORTER",
		"no-mdl-importer" : "ASSIMP_BUILD_NO_MDL_IMPORTER",
		"no-md2-importer" : "ASSIMP_BUILD_NO_MD2_IMPORTER",
		"no-ply-importer" : "ASSIMP_BUILD_NO_PLY_IMPORTER",
		"no-ase-importer" : "ASSIMP_BUILD_NO_ASE_IMPORTER",
		"no-obj-importer" : "ASSIMP_BUILD_NO_OBJ_IMPORTER",
		"no-hmp-importer" : "ASSIMP_BUILD_NO_HMP_IMPORTER",
		"no-smd-importer" : "ASSIMP_BUILD_NO_SMD_IMPORTER",
		"no-mdc-importer" : "ASSIMP_BUILD_NO_MDC_IMPORTER",
		"no-md5-importer" : "ASSIMP_BUILD_NO_MD5_IMPORTER",
		"no-stl-importer" : "ASSIMP_BUILD_NO_STL_IMPORTER",
		"no-lwo-importer" : "ASSIMP_BUILD_NO_LWO_IMPORTER",
		"no-dxf-importer" : "ASSIMP_BUILD_NO_DXF_IMPORTER",
		"no-nff-importer" : "ASSIMP_BUILD_NO_NFF_IMPORTER",
		"no-raw-importer" : "ASSIMP_BUILD_NO_RAW_IMPORTER",
		"no-off-importer" : "ASSIMP_BUILD_NO_OFF_IMPORTER",
		"no-ac-importer" : "ASSIMP_BUILD_NO_AC_IMPORTER",
		"no-bvh-importer" : "ASSIMP_BUILD_NO_BVH_IMPORTER",
		"no-irrmesh-importer" : "ASSIMP_BUILD_NO_IRRMESH_IMPORTER",
		"no-irr-importer" : "ASSIMP_BUILD_NO_IRR_IMPORTER",
		"no-q3d-importer" : "ASSIMP_BUILD_NO_Q3D_IMPORTER",
		"no-b3d-importer" : "ASSIMP_BUILD_NO_B3D_IMPORTER",
		"no-collada-importer" : "ASSIMP_BUILD_NO_COLLADA_IMPORTER",
		"no-terragen-importer" : "ASSIMP_BUILD_NO_TERRAGEN_IMPORTER",
		"no-csm-importer" : "ASSIMP_BUILD_NO_CSM_IMPORTER",
		"no-3d-importer" : "ASSIMP_BUILD_NO_3D_IMPORTER",
		"no-lws-importer" : "ASSIMP_BUILD_NO_LWS_IMPORTER",
		"no-ogre-importer" : "ASSIMP_BUILD_NO_OGRE_IMPORTER",
		"no-ms3d-importer" : "ASSIMP_BUILD_NO_MS3D_IMPORTER",
		"no-cob-importer" : "ASSIMP_BUILD_NO_COB_IMPORTER",
		"no-blend-importer" : "ASSIMP_BUILD_NO_BLEND_IMPORTER",
		"no-q3bsp-importer" : "ASSIMP_BUILD_NO_Q3BSP_IMPORTER",
		"no-ndo-importer" : "ASSIMP_BUILD_NO_NDO_IMPORTER",
		"no-ifc-importer" : "ASSIMP_BUILD_NO_IFC_IMPORTER",
		"no-xgl-importer" : "ASSIMP_BUILD_NO_XGL_IMPORTER",
		"no-fbx-importer" : "ASSIMP_BUILD_NO_FBX_IMPORTER"
	}

	for key, value in data.iteritems():
		if hasattr(arguments, key) and getattr(arguments, key, False):
			product.defines.append(data[key])


def arguments(parser):
	parser.add_argument("--enable-static", 			dest="staticlib", 			action="store_true", default=False, 	help="Build a static (.a) version of the library")
	parser.add_argument("--boost-workaround", 		dest="boost_workaround", 	action="store_true", default=True, 		help="Build a non-boost version of Assimp")
	parser.add_argument("--enable-export", 			dest="enable_export", 		action="store_true", default=False, 	help="Enable Assimp's export functionality")
	parser.add_argument("--enable-tools", 			dest="enable_tools", 		action="store_true", default=False, 	help="Build supplementary tools for assimp")
	parser.add_argument("--enable-samples", 		dest="enable_samples", 		action="store_true", default=False, 	help="Build samples for assimp")
	parser.add_argument("--enable-tests", 			dest="enable_tests", 		action="store_true", default=False, 	help="Build tests for assimp")	

	# arguments which control the various importers. these are all included by default
	# so if you want to omit one or more, you must specify these parameters.
	# 
	parser.add_argument("--no-x-importer",			dest="no-x-importer",		action="store_true", default=True,		help="Omit the X file importer")
	parser.add_argument("--no-3ds-importer",		dest="no-3ds-importer",		action="store_true", default=True,		help="Omit the 3DS file importer")
	parser.add_argument("--no-md3-importer",		dest="no-md3-importer",		action="store_true", default=True,		help="Omit the MD3 file importer")
	parser.add_argument("--no-mdl-importer",		dest="no-mdl-importer",		action="store_true", default=True,		help="Omit the MDL file importer")
	parser.add_argument("--no-md2-importer",		dest="no-md2-importer",		action="store_true", default=True,		help="Omit the MD2 file importer")
	parser.add_argument("--no-ply-importer",		dest="no-ply-importer",		action="store_true", default=True,		help="Omit the PLY file importer")
	parser.add_argument("--no-ase-importer",		dest="no-ase-importer",		action="store_true", default=True,		help="Omit the ASE file importer")
	parser.add_argument("--no-obj-importer",		dest="no-obj-importer",		action="store_true", default=True,		help="Omit the OBJ file importer")
	parser.add_argument("--no-hmp-importer",		dest="no-hmp-importer",		action="store_true", default=True,		help="Omit the HMP file importer")
	parser.add_argument("--no-smd-importer",		dest="no-smd-importer",		action="store_true", default=True,		help="Omit the SMD file importer")
	parser.add_argument("--no-mdc-importer",		dest="no-mdc-importer",		action="store_true", default=True,		help="Omit the MDC file importer")
	parser.add_argument("--no-md5-importer",		dest="no-md5-importer",		action="store_true", default=True,		help="Omit the MD5 file importer")
	parser.add_argument("--no-stl-importer",		dest="no-stl-importer",		action="store_true", default=True,		help="Omit the STL file importer")
	parser.add_argument("--no-lwo-importer",		dest="no-lwo-importer",		action="store_true", default=True,		help="Omit the LWO file importer")
	parser.add_argument("--no-dxf-importer",		dest="no-dxf-importer",		action="store_true", default=True,		help="Omit the DXF file importer")
	parser.add_argument("--no-nff-importer",		dest="no-nff-importer",		action="store_true", default=True,		help="Omit the NFF file importer")
	parser.add_argument("--no-raw-importer",		dest="no-raw-importer",		action="store_true", default=True,		help="Omit the RAW file importer")
	parser.add_argument("--no-off-importer",		dest="no-off-importer",		action="store_true", default=True,		help="Omit the OFF file importer")
	parser.add_argument("--no-ac-importer",			dest="no-ac-importer",		action="store_true", default=True,		help="Omit the AC file importer")
	parser.add_argument("--no-bvh-importer",		dest="no-bvh-importer",		action="store_true", default=True,		help="Omit the BVH file importer")
	parser.add_argument("--no-irrmesh-importer",	dest="no-irrmesh-importer",	action="store_true", default=True,		help="Omit the irrmesh file importer")
	parser.add_argument("--no-irr-importer",		dest="no-irr-importer",		action="store_true", default=True,		help="Omit the irr file importer")
	parser.add_argument("--no-q3d-importer",		dest="no-q3d-importer",		action="store_true", default=True,		help="Omit the Q3D file importer")
	parser.add_argument("--no-b3d-importer",		dest="no-b3d-importer",		action="store_true", default=True,		help="Omit the b3d file importer")
	parser.add_argument("--no-collada-importer",	dest="no-collada-importer",		action="store_true", default=True,		help="Omit the COLLADA file importer")
	parser.add_argument("--no-terragen-importer",	dest="no-terragen-importer",		action="store_true", default=True,		help="Omit the terragen file importer")
	parser.add_argument("--no-csm-importer",		dest="no-csm-importer",		action="store_true", default=True,		help="Omit the csm file importer")
	parser.add_argument("--no-3d-importer",			dest="no-3d-importer",		action="store_true", default=True,		help="Omit the 3d file importer")
	parser.add_argument("--no-lws-importer",		dest="no-lws-importer",		action="store_true", default=True,		help="Omit the lws file importer")
	parser.add_argument("--no-ogre-importer",		dest="no-ogre-importer",		action="store_true", default=True,		help="Omit the ogre file importer")
	parser.add_argument("--no-ms3d-importer",		dest="no-ms3d-importer",		action="store_true", default=True,		help="Omit the ms3d file importer")
	parser.add_argument("--no-cob-importer",		dest="no-cob-importer",		action="store_true", default=True,		help="Omit the cob file importer")
	parser.add_argument("--no-blend-importer",		dest="no-blend-importer",		action="store_true", default=True,		help="Omit the blend file importer")
	parser.add_argument("--no-q3bsp-importer",		dest="no-q3bsp-importer",		action="store_true", default=True,		help="Omit the q3bsp file importer")
	parser.add_argument("--no-ndo-importer",		dest="no-ndo-importer",		action="store_true", default=True,		help="Omit the ndo file importer")
	parser.add_argument("--no-ifc-importer",		dest="no-ifc-importer",		action="store_true", default=True,		help="Omit the ifc file importer")
	parser.add_argument("--no-xgl-importer",		dest="no-xgl-importer",		action="store_true", default=True,		help="Omit the xgl file importer")
	parser.add_argument("--no-fbx-importer",		dest="no-fbx-importer",		action="store_true", default=True,		help="Omit the FBX file importer")

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

	set_importer_flags(arguments, product)

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


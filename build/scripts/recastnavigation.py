import logging
from pegasus.models import Product, ProductType

def create_product(name, directory, target_platform):

	output_type = ProductType.StaticLibrary if target_platform.matches("windows") else ProductType.DynamicLibrary

	product = Product(name=name, output=output_type)
	product.root = "../dependencies/recastnavigation"
	product.project_root = "_projects"
	product.object_root = "${PROJECT_ROOT}/obj"
	product.excludes = [
		"*.DS_Store"
	]

	product.sources += [
		"%s/Source/*.cpp" % directory,
		"%s/Include/*.h" % directory
	]
	product.includes += [
		"%s/Include" % directory
	]	

	debug = product.layout(configuration="debug")
	debug.defines += [
		"DEBUG"
	]

	return product

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	recast = create_product("recast", "Recast", target_platform)
	detour = create_product("detour", "Detour", target_platform)
	detourcrowd = create_product("detourcrowd", "DetourCrowd", target_platform)
	detourcrowd.dependencies.append(detour)

	detourtilecache = create_product("detourtilecache", "DetourTileCache", target_platform)
	detourtilecache.dependencies.append(detour)

	debugutils = create_product("debugutils", "DebugUtils", target_platform)
	debugutils.dependencies.append(detour)
	debugutils.dependencies.append(recast)
	debugutils.dependencies.append(detourtilecache)

	return [debugutils, detourtilecache, detourcrowd, detour, recast]
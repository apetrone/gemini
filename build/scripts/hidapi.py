import logging
from pegasus.models import Product, ProductType


def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)

	output_type = ProductType.StaticLibrary if target_platform.matches("windows") else ProductType.DynamicLibrary

	product = Product(name='hidapi', output=output_type)
	product.root = "../dependencies/hidapi"
	product.project_root = "_projects"
	if target_platform.matches("windows"):
		product.sources += [
			"windows/hid.c"
		]
	elif target_platform.matches("linux"):
		product.sources += [
			"linux/hid.c"
		]
	elif target_platform.matches("macosx"):
		product.sources += [
			"mac/hid.c"
		]

	product.includes += [
		"hidapi",
	]	

	debug = product.layout(configuration="debug")
	debug.defines += [
		"DEBUG"
	]

	return [product]
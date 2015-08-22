import logging
from pegasus.models import Product, ProductType

def arguments(parser):
	pass

def products(arguments, **kwargs):
	rapidjson = Product(name="rapidjson", output=ProductType.None)
	rapidjson.root = "../dependencies/rapidjson"
	rapidjson.includes = [
		"include"
	]

	return [rapidjson]


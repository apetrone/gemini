import logging

from pegasus.models import Product, ProductType

def products(arguments, **kwargs):
	sqrat = Product(name="sqrat", output=ProductType.None)
	sqrat.root = "../dependencies/sqrat"
	sqrat.includes = [
		"include"
	]

	return [sqrat]


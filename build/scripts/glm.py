import logging

from pegasus.models import Product, ProductType

def products(arguments, **kwargs):
	glm = Product(name="glm", output=ProductType.None)
	glm.root = "../dependencies/glm"
	glm.includes = [
		"."
	]

	return [glm]


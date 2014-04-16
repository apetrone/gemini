import os
import logging

from pegasus.models import Product, ProductType, Dependency

DEPENDENCIES_FOLDER = "dependencies"

def setup_common_variables(target_platform, product):
	product.sources = [
		"src/*.c*",
		"src/*.h*",

		# dependencies
		os.path.join(DEPENDENCIES_FOLDER, "murmur3/murmur3.c"),

		# include this almagamated version of jsoncpp until we replace it.
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp/jsoncpp.cpp"),

		os.path.join(DEPENDENCIES_FOLDER, "font-stash/fontstash.c"),
		os.path.join(DEPENDENCIES_FOLDER, "font-stash/stb_truetype.c"),

		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.c"),
		os.path.join(DEPENDENCIES_FOLDER, "slim/slim/*.h")
	]


	product.includes = [
		"src",
		"src/game",

		os.path.join(DEPENDENCIES_FOLDER, "murmur3"),
		os.path.join(DEPENDENCIES_FOLDER, "jsoncpp"),
		os.path.join(DEPENDENCIES_FOLDER, "slim")
	]

	product.excludes = [
		"src/entry.cpp"
	]

	product.defines = [
		"JSON_IS_AMALGAMATION"
	]

	debug = product.layout(configuration="debug")
	release = product.layout(configuration="release")

	debug.defines = [
		"DEBUG"
	]


def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform")

	gemini = Product(name="desktop", output=ProductType.Application)
	gemini.root = "../"
	gemini.product_root = "latest/bin/${CONFIGURATION}_${ARCHITECTURE}"
	gemini.project_root = "_projects"
	gemini.object_root = "${PROJECT_ROOT}/obj"


	gemini.dependencies = [
		Dependency(file="glm.py"),
		Dependency(file="sqrat.py"),
		Dependency(file="squirrel3.py")
	]

	if target_platform.get() in ["macosx", "linux", "windows"]:
		gemini.dependencies += [
			Dependency(file="xwl.py", products=["xwl"])
		]

	# general sources
	setup_common_variables(target_platform, gemini)

	#
	# Dependencies in source form
	#


	return [gemini]


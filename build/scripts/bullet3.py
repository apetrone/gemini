import logging

from pegasus.models import Product, ProductType

class BuildSettings(object):
	def __init__(self, target_platform):
		self.target_platform = target_platform

	def populate(self, product):
		product.root = "../dependencies/bullet3"
		product.project_root = "_projects"
		product.object_root = "${PROJECT_ROOT}/obj"

		#
		# macosx
		#
		product.driver.gcc_symbols_private_extern = "YES"
		product.driver.gcc_inlines_are_private_extern = "YES"

		mac_debug = product.layout(configuration="debug", platform="macosx")
		mac_debug.driver.gcc_optimization_level = "0"
		mac_debug.driver.gcc_generate_debugging_symbols = "YES"
		mac_debug.driver.debug_information_format = "dwarf-with-dsym"

		mac_release = product.layout(configuration="release", platform="macosx")
		mac_release.driver.gcc_optimization_level = "2"
		mac_release.driver.gcc_generate_debugging_symbols = "NO"

def BulletSoftBody(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	sb = Product(name="BulletSoftBody", output=ProductType.StaticLibrary)
	sb.includes = [
		"src"
	]

	sb.sources = [
		"src/BulletSoftBody/*.cpp",
		"src/BulletSoftBody/*.h"
	]

	settings.populate(sb)
	return sb



def BulletDynamics(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	bd = Product(name="BulletDynamics", output=ProductType.StaticLibrary)
	bd.includes = [
		"src"
	]

	bd.sources = [
		"src/BulletDynamics/Character/*.*",
		"src/BulletDynamics/ConstraintSolver/*.*",
		"src/BulletDynamics/Dynamics/*.*",
		"src/BulletDynamics/Vehicle/*.*"
	]

	settings.populate(bd)
	return bd


def BulletCollision(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	bc = Product(name="BulletCollision", output=ProductType.StaticLibrary)
	bc.includes = [
		"src"
	]

	bc.sources = [
		"src/BulletCollision/BroadphaseCollision/*.*",
		"src/BulletCollision/CollisionDispatch/*.*",
		"src/BulletCollision/CollisionShapes/*.*",
		"src/BulletCollision/Gimpact/*.*",
		"src/BulletCollision/NarrowPhaseCollision/*.*"
	]

	settings.populate(bc)
	return bc

def LinearMath(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	lm = Product(name="LinearMath", output=ProductType.StaticLibrary)
	lm.includes = [
		"src"
	]

	lm.sources = [
		"src/LinearMath/*.cpp",
		"src/LinearMath/*.h"
	]

	settings.populate(lm)
	return lm


def BulletMultiThreaded(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	mt = Product(name="BulletMultiThreaded", output=ProductType.StaticLibrary)
	mt.includes = [
		"src"
	]

	mt.sources = [
		"src/BulletMultiThreaded/*.cpp",
		"src/BulletMultiThreaded/*.h",
		"src/BulletMultiThreaded/SpuNarrowPhaseCollisionTask/*.cpp",
		"src/BulletMultiThreaded/SpuNarrowPhaseCollisionTask/*.h"		
	]

	settings.populate(mt)
	return mt



def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = BuildSettings(target_platform)

	kwargs["buildsettings"] = settings

	softbody = BulletSoftBody(arguments, **kwargs)

	dynamics = BulletDynamics(arguments, **kwargs)
	
	collision = BulletCollision(arguments, **kwargs)
	
	linearmath = LinearMath(arguments, **kwargs)

	multithreaded = BulletMultiThreaded(arguments, **kwargs)


	collision.dependencies = [linearmath]
	dynamics.dependencies = [linearmath]


	return [softbody, dynamics, collision, linearmath, multithreaded]

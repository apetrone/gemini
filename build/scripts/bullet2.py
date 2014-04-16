import logging

from pegasus.models import Product, ProductType

class BuildSettings(object):
	def __init__(self, target_platform):
		self.target_platform = target_platform

	def populate(self, product):
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

def LinearMath(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	lm = Product(name="LinearMath", output=ProductType.StaticLibrary)
	lm.root = "../dependencies/bullet2"
	lm.includes = [
		"src"
	]

	lm.sources = [
		"src/LinearMath/*.cpp",
		"src/LinearMath/*.h"
	]

	settings.populate(lm)
	return lm

def BulletCollision(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	bc = Product(name="BulletCollision", output=ProductType.StaticLibrary)
	bc.root = "../dependencies/bullet2"
	bc.includes = [
		"src"
	]

	bc.sources = [
		"src/BulletCollision/BroadphaseCollision/*.*",
		"src/BulletCollision/CollisionDispatch/*.*",
		"src/BulletCollision/CollisionShapes/*.*",
	]

	settings.populate(bc)
	return bc

def BulletDynamics(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	bd = Product(name="BulletDynamics", output=ProductType.StaticLibrary)
	bd.root = "../dependencies/bullet2"
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

def BulletSoftBody(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	sb = Product(name="BulletSoftBody", output=ProductType.StaticLibrary)
	sb.root = "../dependencies/bullet2"
	sb.includes = [
		"src"
	]

	sb.sources = [
		"src/BulletSoftBody/*.cpp",
		"src/BulletSoftBody/*.h"
	]

	settings.populate(sb)
	return sb


def BulletMultiThreaded(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)
	settings = kwargs.get("buildsettings", None)

	mt = Product(name="BulletMultiThreaded", output=ProductType.StaticLibrary)
	mt.root = "../dependencies/bullet2"
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

	linearmath = LinearMath(arguments, **kwargs)
	collision = BulletCollision(arguments, **kwargs)
	collision.dependencies = [linearmath]

	dynamics = BulletDynamics(arguments, **kwargs)
	dynamics.dependencies = [linearmath]


	softbody = BulletSoftBody(arguments, **kwargs)

	multithreaded = BulletMultiThreaded(arguments, **kwargs)

	return [linearmath, collision, dynamics, softbody, multithreaded]

import os
import logging
from pegasus.models import Product, ProductType

def arguments(parser):
	pass

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform", None)

	openvr = Product(name="openvr", output=ProductType.StaticLibrary)
	openvr.root ="../dependencies/openvr"

	if target_platform.matches("windows"):
		# need to use its own folder due to line endings
		# until I move that to a better solution
		#ovr.root = os.path.join(SDK_ROOT, "oculussdk_%s_windows" % LIBOVR_VERSION)
		openvr.name = "openvr_api" # use libovr/d or libovr64/d
		openvr.product_root = "lib/win64" # use win32 or win64 for platform
	else:
		raise Exception("OpenVR is only setup for windows at the present.")

	openvr.includes = [
		"headers"
	]

	return [openvr]
import logging

from pegasus.models import Product, ProductType, Dependency

def products(arguments, **kwargs):
	target_platform = kwargs.get("target_platform")

	freetype = Product(name="freetype", output=ProductType.DynamicLibrary)
	freetype.root = "../dependencies/freetype-2.4.2"
	freetype.project_root = "_projects"
	freetype.sources += [
		"src/autofit/autofit.c",
		"src/bdf/bdf.c",
		"src/cff/cff.c",
		"src/base/ftbase.c",
		"src/base/ftbitmap.c",
		"src/cache/ftcache.c",
		"src/base/ftfstype.c",
		"src/base/ftgasp.c",
		"src/base/ftglyph.c",
		"src/gzip/ftgzip.c",
		"src/base/ftinit.c",
		"src/lzw/ftlzw.c",
		"src/base/ftstroke.c",
		"src/base/ftsystem.c",
		"src/smooth/smooth.c",
		"src/base/ftbbox.c",
		"src/base/ftmm.c",
		"src/base/ftpfr.c",
		"src/base/ftsynth.c",
		"src/base/fttype1.c",
		"src/base/ftlcdfil.c",
		"src/base/ftgxval.c",
		"src/base/ftotval.c",
		"src/base/ftpatent.c",
		"src/base/ftxf86.c",
		"src/pcf/pcf.c",
		"src/pfr/pfr.c",
		"src/psaux/psaux.c",
		"src/pshinter/pshinter.c",
		"src/psnames/psmodule.c",
		"src/raster/raster.c",
		"src/sfnt/sfnt.c",
		"src/truetype/truetype.c",
		"src/type1/type1.c",
		"src/cid/type1cid.c",
		"src/type42/type42.c",
		"src/winfonts/winfnt.c",

		"include/ft2build.h",
		"include/freetype/config/ftconfig.h",
		"include/freetype/config/ftheader.h",
		"include/freetype/config/ftmodule.h",
		"include/freetype/config/ftoption.h",
		"include/freetype/config/ftstdlib.h"
	]
	freetype.includes += [
		"include"
	]
	freetype.defines += [
		"FT2_BUILD_LIBRARY=1"
	]

	debug = freetype.layout(configuration="debug")
	debug.defines += [
		"_DEBUG",
		"FT_DEBUG_LEVEL_ERROR",
		"FT_DEBUG_LEVEL_TRACE"
	]
	release = freetype.layout(configuration="release")
	release.defines += [
		"NDEBUG"
	]

	#
	# per-platform configuration
	#
	#
	macosx = freetype.layout(platform="macosx")
	macosx.sources += [
		"src/base/ftdebug.c"
	]
	macosx.includes += [
		"builds/ansi",
		"objs"
	]
	macosx.defines += [
		"FT_MACINTOSH=1",
		"DARWIN_NO_CARBON",
		"FT_CONFIG_MODULES_H=<ftmodule.h>"
	]

	linux = freetype.layout(platform="linux")
	linux.sources += [
		"src/base/ftdebug.c"
	]
	linux.includes += [
		"builds/unix",
		"config"
	]
	linux.defines += [
		"FT_CONFIG_CONFIG_H=\"<ftconfig.h>\"",
		"FT_CONFIG_MODULES_H=\"<freetype/config/ftmodule.h>\""
	]

	if target_platform.matches("linux"):
		freetype.copy_config("include/freetype/config/ftconfig.h", "builds/unix/ftconfig.h")

	windows = freetype.layout(platform="windows")
	windows.sources += [
			"builds/win32/ftdebug.c",
			"src/base/ftwinfnt.c",
	]
	windows.defines += [
		"WIN32",
		"_LIB",
		"_CRT_SECURE_NO_WARNINGS",
		"_CRT_SECURE_NO_DEPRECATE"
	]

	return [freetype]


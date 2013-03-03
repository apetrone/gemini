BUILD_NAME = "gemini"
BUILD_ROOT = "build"

def postbuild_common():
	logging.info( "Hello" )


def construct_binpath(params):
	return "latest/bin/%s/%s" % (params['build_architecture'], params['configuration'])

def dependency_libpath():
	return "lib/{architecture}/{configuration}"

def dependency_path():
	return "build/dependencies"

def common_dependencies():
	return [
	"soil.py", 
	"glm.py"
	]
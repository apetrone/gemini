BUILD_NAME = "gemini"
BUILD_ROOT = "build"

def construct_binpath(params):
	return "latest/bin/%s/%s" % (params['build_architecture'], params['configuration'])

def dependency_libpath():
	return "lib/{architecture}/{configuration}"

def dependency_path():
	return "build/dependencies"

def common_dependencies( target_platform ):
	deps = [
		"glm.py",
		"fontstash.py",
		#"bullet2.py",
		"squirrel3.py",
		"sqrat.py",
		"nom/nom.py",
		"box2d.py"
	]

	if target_platform == 'windows':
		deps.append( "openal.py" )

	return deps
# Adam Petrone
# August, 2014

import os
import sys
import logging
import subprocess

BOOTSTRAP_VIRTUALENV_PATH = "env"
REQUIREMENTS_FILE = "requirements"

def get_virtualenv_path(root_path, name):
	# if the system is posix, the virtualenv binaries are placed
	# into a "bin" folder. Windows places these into "Scripts"

	intermediate_paths = {
		"posix": "bin",
		"nt": "Scripts"
	}

	extensions = {
		"posix": "",
		"nt": ".exe"
	}

	path = intermediate_paths[os.name]
	binary_name = name + extensions[os.name]

	return os.path.join(root_path, path, binary_name)

def setup_environment(after_install):
	try:
		import virtualenv
	except:
		raise Exception("virtualenv not installed! This is required.")

	root_path = os.path.dirname(__file__)
	virtualenv_root = os.path.join(root_path, BOOTSTRAP_VIRTUALENV_PATH)

	if os.path.exists(virtualenv_root):
		logging.info(
			"virtualenv already exists at \"%s\". Nothing to do." %
			 virtualenv_root
		)
		return virtualenv_root

	logging.info("creating virtualenv at \"%s\"" % virtualenv_root)
	sys.argv.append("--distribute")
	sys.argv.append(virtualenv_root)
	virtualenv.after_install = after_install
	virtualenv.main()
	return virtualenv_root

def install_packages(root_path):
	pip = get_virtualenv_path(root_path, "pip")
	abs_requirements_path = os.path.abspath(
		os.path.join(root_path, os.path.pardir, REQUIREMENTS_FILE)
	)
	command = [pip, "install", "-r", abs_requirements_path]
	subprocess.call(command)

def build_docs(root_path):
	sphinx_build = get_virtualenv_path(root_path, "sphinx-build")
	command = [
		sphinx_build,
		"-b",
		"html",
		"docs",
		"docs/html"
	]
	subprocess.call(command)


def post_install(options, root_path):
	# after the virtualenv is installed, call the following
	# 
	# install via requirements file
	install_packages(root_path)

if __name__ == "__main__":
	logging.basicConfig(level=logging.INFO)
	root_path = setup_environment(post_install)

	# (this should be moved) build documentation
	build_docs(root_path)	

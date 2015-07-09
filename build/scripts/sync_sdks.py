# author: Adam Petrone
# date: March 2015

import os
import sys
import logging
import subprocess

# This script will sync the private sdks
# from the build server to the correct location.

# This is NOT a submodule because in many cases
# we don't want sync these, as these are large,
# cumbersome sdks (fbx) and some private sdks
# which don't apply on all platforms.

DEPENDENCIES_ROOT = os.path.join("build", "dependencies")
TARGET_LOCATION = os.path.join(DEPENDENCIES_ROOT, "sdks")
BUILD_SERVER_HOST = "solo"

if __name__ == "__main__":
	logging.basicConfig(level=logging.INFO)

	# the dependency root folder should already exist
	if not os.path.exists(DEPENDENCIES_ROOT):
		raise Exception("This script must be run from the repository root!")

	if not os.path.exists(TARGET_LOCATION):
		logging.info("Cloning SDKs repo from git...")
		try:
			command = [
				"git",
				"clone",
				"git://%s/gemini/sdks.git" % BUILD_SERVER_HOST,
				TARGET_LOCATION
			]

			subprocess.call(command)
		except:
			raise

		logging.info("Completed SDK sync.")
	else:
		logging.info("Nothing to do. '%s' exists" % TARGET_LOCATION)

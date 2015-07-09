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

TARGET_LOCATION = "build/dependencies/sdks"
BUILD_SERVER_HOST = "solo"

if __name__ == "__main__":
	logging.basicConfig(level=logging.INFO)

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

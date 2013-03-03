gemini


Murmur3 is in the public domain. Murmur3 was created by Austin Appleby, and the C port and general tidying up was done by Peter Scott.


Clone the repository

	git clone git://github.com/apetrone/gemini

Grab all sourcecode dependencies to build:

	git submodule update --init

The pegasus tool is used to build gemini for various platforms and configurations.

	pegasus -t gemini.py -b debug,release -c build

For the RaspberryPi:

	pegasus -t raspberrypi.py -b debug,release -c build -p

For iOS

	pegasus -t iphoneos.py -b debug,release -c build -p 
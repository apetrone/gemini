gemini


Murmur3 is in the public domain. Murmur3 was created by Austin Appleby, and the C port and general tidying up was done by Peter Scott.


#### Clone the repository

		git clone git://github.com/apetrone/gemini

#### Grab all sourcecode dependencies to build:

		git submodule update --init

#### Generate project files with pegasus

		python tools/pegasus/build.py build/scripts/gemini.py generate

#### Build project files with pegasus

		python tools/pegasus/build.py build/scripts/gemini.py build


### Installation Notes

#### Linux
	
### Debian/Ubuntu/SteamOS

		sudo apt-get install sox libasound-dev


### Arch Linux

		pacman -S extra/openal alsa-lib


### Principles / Goals
* git
	* Everything needed to "build" the game, will be in git.
	* Art assets and other content will be in another system (though there are some strays in this project at the moment.)

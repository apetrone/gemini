#### ideology
This project is my effort at making a cross-platform game framework.

It's basically a collection of everything I've learnt on my own in the
realm of game development. That being said, everything is based on hobby
experience and this entire framework is considered "in development".

I want to eventually make games with this, but there are certain aspects
of game creation I refuse to compromise on. I'd rather spend the time to 
work on a better solution to those than create another throwaway game.

That being said, the direction on this project may confuse some. 
"Make Games, Not Engines", I hear you cry. Yes -- if my intent was to
create a game, I would be working on that instead. However, I find no
satisfaction in working on a game with a pre-existing toolset or engine.
All the technical decisions have been made for me -- and most are obfuscated
behind a button. 

I enjoy working on tech and learning how everything functions. That drives
the direction of this framework and the choices I make. Usually this means
I take a minimalist approach to the problems and increase complexity
when needed.


#### licensing

See license.md for details.


#### supported platforms

- Linux (Ubuntu, Arch Linux)
- MacOS X
- Windows (sigh)
- iOS
- Android


#### building

The general steps to building are as follows:

1. Clone the repository and submodules
2. Generate projects for your target platform
3. Build the code
4. Deploy to your target platform


* Linux requires some external dependencies which can be installed
via your distro's package manager.

	*  Debian/Ubuntu/SteamOS

			sudo apt-get install sox libasound-dev


	* Arch Linux

			pacman -S extra/openal alsa-lib


* Clone the repository and get submodules

		git clone git://github.com/apetrone/gemini
		git submodule update --init	--recursive

* Generate project files with pegasus (use --help for additional options)

	* Desktop (Linux / MacOS / Windows)

			python tools/pegasus/build.py build/scripts/gemini.py generate

	* iOS

			python tools/pegasus/build.py build/scripts/gemini.py generate -p iphone

	* Android

			python tools/pegasus/build.py build/scripts/gemini.py generate -p android

* Build project files with pegasus

		python tools/pegasus/build.py build/scripts/gemini.py build


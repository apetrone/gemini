.. _Linux: build-linux_
.. _MacOS X: build-desktop_
.. _Windows: build-windows_
.. _iOS: build-mobile_
.. _Android: build-mobile_

Compiling/Building
--------------------------

The general steps to building are as follows:

1. Fetch all submodules
2. Generate projects for your target platform
3. Build the libraries, tools, etc.
4. Package the game + assets for your target platform
5. Deploy to your target platform

Jump to your target platform.

1. Linux_ (including embedded devices)
2. `MacOS X`_
3. `Windows`_
4. `iOS`_
5. `Android`_

----


-------------------
Fetch submodules
-------------------

	.. code-block:: bash

		git submodule update --init --recursive

-------------------
Generate Projects
-------------------

Project generation is achieved with pegasus. Use --help for additional options.
This varies per target platform due to different options such as rendering
API, architecture differences, etc.

.. _build-linux:

On Linux, we take advantage of the package managers in order to fetch
dependencies. This will vary based on your distro's package manager and
must be performed before attempting to generate projects.

Debian/Ubuntu/SteamOS

	GCC 4.9.0+ is required. (A dependency based on C++ regex.)

	Known issues with distributions:
		- Xubuntu 12.04 (LTS): Does not build. Incompatibility with C++ regex_replace.

	Successfully tested distributions:
		- Kubuntu (version?)
		- Xubuntu 14.04.5 (LTS)

	.. code-block:: bash

		sudo apt-get install sox libasound2-dev libudev-dev

	If building with X11; you also need X11 libs.

	.. code-block:: bash

		sudo apt-get install libx11-dev libxinerama-dev mesa-common-dev

	You may also need to symlink libGL to /usr/lib/libGL.so. Find it with:

	.. code-block:: bash

		locate libGL

Arch Linux

	.. code-block:: bash

		pacman -S alsa-lib

These optional variables should be specified when building for the RaspberryPi.

	1. --indextype 2
	2. --build-raspberry-pi


.. _build-windows:

On Windows, unfortunately, the DirectX SDK must be installed. As of this writing, SDL2 requires it.



----------------------------
Building for Desktop
----------------------------


.. _build-desktop:

Desktop (Linux / MacOS / Windows)
	.. code-block:: bash

		python tools/pegasus/build.py build/scripts/gemini.py generate

--------------------
Building for Mobile
--------------------

.. _build-mobile:

iOS
	.. code-block:: bash

		python tools/pegasus/build.py build/scripts/gemini.py generate -p iphone

Android
	.. code-block:: bash

		python tools/pegasus/build.py build/scripts/gemini.py generate -p android

Building for mobile platforms is nearly identical to building for the desktop.
These following options will help you configure the


The supported platforms are as follows:


Build project files with pegasus

	.. code-block:: bash

		python tools/pegasus/build.py build/scripts/gemini.py build


----------
Packaging
----------

Packaging will likely be achieved through the use of included python
scripts.

-----------
Deployment
-----------

Desktop platforms can take advantage of deployments via precache.

On mobile devices, packages from the previous step will allow deployments
to the appropriate online distribution mechanisms.

Developer 
-------------------

Removing a git submodule:

- deinit the submodule; remove modules folder and submodule from cache.
	.. code-block:: c
	
		git submodule deinit -f <submodule>
		rm -rf .git/modules/<submodule>
		git rm --cached <submodule>

- edit the .gitmodules file and remove traces of the module
	.. code-block:: c

		nano .gitmodules
		...

- ensure the submodules are consistent (should produce no errors)
	.. code-block:: c
	
		git submodule sync
		git submodule update

Source Code Organization:

- 'core': containers, utilities, memory management, math
- 'platform': platform abstraction layer
- 'runtime': assets, logging, filesystem services
- 'renderer': a platform independent rendering layer




Specialty Builds:

- building on the Raspberry Pi:
	required packages: openal
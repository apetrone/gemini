Developer 
-------------------

Removing a git submodule:

- deinit the submodule; remove modules folder and submodule from cache.
	.. code-block:: c
	
		git submodule deinit -f <submodule>
		rm .git/modules/<submodule>
		rm --cached <submodule>

- edit the .gitmodules file and remove traces of the module
	.. code-block:: c

		nano .gitmodules
		...

- ensure the submodules are consistent (should produce no errors)
	.. code-block:: c
	
		git submodule sync
		git submodule update




Code Style Guidelines
---------------------

*Please note, this project is in a major transitional period to the following
being discussed in this document.*

---------
Overview
---------

Consistency is a priority for determinism and overall developer sanity.
With consistency, developers can use intuition instead of constantly
referencing documentation. The following rules are used in various capacities
to communicate the desired result.

- Loops
	Use meaningful variable names: i, iter, index, to indicate interation.
	The tuples i,j,k or x,y,z may be substituted where multiple variables are needed in nested conditions.

- Preprocessor Macros
	a) For generic, portable, or otherwise re-usable code -- use generic names.
		This applies to defines for the platforms, which are PLATFORM_MOBILE or PLATFORM_LINUX.

	b) Project-specific code can use the codenames in the macros.
		.. code-block:: c

			#define GEMINI_ENABLE_TEST 1

- File naming conventions
	* C/C++ Source files are .c or .cpp. Headers are .h (again, transitional on this one)
	* C code must support C++ compiles (guard in extern "C")

- Consistent Terminology
	One principle of solid API design is consistency and intuition. If your user knows your API, they can begin to infer how to use it.

	Here, we'll create general categories and outline usage which should be changed (for non-compliant code) and serve as a baseline for future additions.

	a) "calculate" or "compute" should be used when there is a computation to be done.
	b) Use "total" to describe an array; e.g. "total_elements", "total_bones"


------------------
Memory Allocation
------------------

- create - construct a new object (calls constructor)
- destroy - delete an object (calls destructor)
- allocate - allocate memory on the heap
- deallocate - deallocates previously allocated memory

-----------------
Member Functions
-----------------

- reset - Used to reset instance state; set variables to zero, free memory, etc.
- init - Used to create instance data


------------
Style Rules
------------

- No extraneous whitespace.

	In summary, this is the old way:

	.. code-block:: c

		void my_function( int a, int b )
		{
			// ...
		}

		my_function( 3, 0 )


	And this is the new way:

	.. code-block:: c

		void my_function(int a, int b)
		{
			// ...
		}

		my_function(3, 0)

	Pointer and reference types should not have a space between the identifier and specifier.
	Additionally, these should be located next to the type, not the variable.
	This should be consistent between normal variable declarations and function signatures.

	.. code-block:: c

		void my_function(int* a, int* b);
		// ...
		int* my_value = &x;

	A space separating parameters is expected (as is the case in normal writing).

	This makes parameters easier to read.


- Variable names (including member variables) are lower-cased with underscores:

	.. code-block:: c

		int my_value = 3;


- Static or global names should be prefixed with an underscore:

	.. code-block:: c

		Kernel* _global_instance = 0;


- Function names are lower cased with underscores for spacing:

	.. code-block:: c

		kernel::instance()->set_active(true);


- Namespace names are lower cased:

	.. code-block:: c++

		namespace core
		{
			// ...
		};


- Struct and class names are title-cased:

	.. code-block:: c++

		class AnimationController
		{
			// ...
		};

	This helps distinguish class and struct names from pod and system/os types.

- Files for classes should be lower-cased and use underscores:

	.. code-block:: c++

		class AnimationController {};
		animation_controller.h
		animation_controller.cpp

	This makes porting code to other platforms easy. It also encourages the use
	of including lower case filenames which makes this work across filesystems regardless
	of case sensitivity.

- Only the following prefixes are added to classes:

	a) [This should be removed] Interface/Abstract classes are prefixed with the letter I.
		The concrete implementation of this interface can merely drop the I.

		.. code-block:: c

			class IPlayerController
			{
			public:
				virtual ~IPlayerController() {}

				virtual void important_function() = 0;
			};

			// ...

			class PlayerController : public IPlayerController
			{
				// ...
			};

	In the event this interface class is placed into its own file,
	the file would also be prefixed with the letter I; lower-cased
	to conform to file naming conventions.

- Curly Braces

	a) Used with all control statements for consistency:

		.. code-block:: c

			int z = 0;
			for(int i = 0; i < 0xFF; ++i)
			{
				z += i;
			}

	b) Placed on separate lines for code legibility:

		.. code-block:: c

			if (was_terminated || has_ended)
			{
				// ...
			}

- Including headers should be in the following order.

	a) local: using double-quotes
	b) external/third-party: using brackets
	c) system: using brackets

	.. code-block:: c

		#include "kernel.h"
		#include <sdk/types.h>
		#include <string>

- Header names should not clash with standard headers.

- Template type names should be kept simple, but representative of their purpose

	.. code-block:: c++

		template <class K, class V>
		class HashTable
		{
			// ...
		};

It keeps code straight forward to read, but also lets you know which types are
template variables.

- Files should be all lower-cased characters.
	This eases the transition across platforms.

	Additionally, underscores can be used to denote break up the name
	to make it easier to read.

	a) As an example:

		.. code-block:: c

			model_api.h
			iplayercontroller.h
			physics_interface.h

- Newlines at the end of header files (C++98 compliance).
	This is a good habit, even though C++11 standards have removed this
	restriction; it's useful when programming C.
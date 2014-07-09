### Code Style Guidelines
-

Table of Contents

I. <a href="#overview">Overview</a><br/>
II. <a href="#source-code-style">Source Code Style</a><br/>

-


#### Overview
* Consistency is a priority for determinism and overall developer sanity.
Please note, this project is in a major transitional period regarding that previous statement.

	The following rules are used in various capacities to communicate the desired result.

	* Loops should use meaningful variable names: i, iter, index, to indicate interation.	
	The tuples i,j,k or x,y,z may be substituted where multiple variables are needed in nested conditions.

	* Preprocessor Macros

		* For generic, portable, or otherwise re-usable code -- use generic names.
		This applies to defines for the platforms, which are PLATFORM_MOBILE or PLATFORM_LINUX.

		* Project-specific code can use the codenames in the macros.
			```c
			#define GEMINI_USE_SDL2 1
			```
	* File naming conventions

		* C/C++ Source files are .c or .cpp. Headers are .h (again, transitional on this one)
		* C code must support C++ compiles (guard in extern "C")

	* Consistent Terminology is important for API design and determinism.

		* calculate or compute should be used when there is a computation to be done.

#### Source Code Style:

- Please note: Code Style guidelines are currently under development and the code
is also in a transitional state. It will take a bit of time to transition everything over.

- No extraneous whitespace.

	In summary, this is the old way:
	```c
	void my_function( int a, int b )
	{
		// ...
	}

	my_function( 3, 0 )
	```

	And this is the new way:
	```c
	void my_function(int a, int b)
	{
		// ...
	}

	my_function(3, 0)
	```

	Pointer and reference types should not have a space between the identifier and specifier.
	Additionally, these should be located next to the type, not the variable.
	This should be consistent between normal variable declarations and function signatures.
	```c
	void my_function(int* a, int* b);

	int* my_value = &x;
	```

	A space separating parameters is expected (as is the case in normal writing).

- variable names (including member variables) are lower-cased with underscores: 

```c
int my_value = 3;
```

- static or global names should be prefixed with an underscore:

```c
Kernel * _global_instance = 0;
```
- function names are lower cased with underscores for spacing:

```c
_kernel->set_active( true );
```
- namespace names are lower cased:

```c
namespace core 
{
	// ...
};
```

- struct and class names are upper-cased:

```c
class AnimationController 
{
	// ...
};
```
No prefixes are added to interface/abstract classes.

- curly braces are explicitly used with all control statements for consistency:

```c
int z = 0;
for(int i = 0; i < 0xFF; ++i)
{
	z += i;
}
```

- curly braces are placed on separate lines for code legibility:

```c
if (was_terminated || has_ended)
{
	// ...
}
```

- Standard headers are always included first, using brackets. Third-party libraries are included next, using brackets as well. Local project includes are always in double-quotes.

```c
#include <string>
#include <sdk/types.hpp>
#include "kernel.hpp"
```

- Template type names are title cased. 
Unlike the STL, these should be moderately descriptive.

```c
template <class KeyType, class ValueType>
class HashTable
{
	// ...
};
```

- Header names should not clash with standard headers. 
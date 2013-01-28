Code Style Guide for Project Gemini
--------

- variable names (including member variables) are lower-cased with underscores: 

```c
int my_value = 3;
```

- static or global names should be prefixed with an underscore:

```c
IKernel * _global_instance = 0;
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
- curly braces are explicitly used with all control statements for consistency:

```c
int z = 0;
for( int i = 0; i < 0xFF; ++i )
{
	z += i;
}
```

- curly braces are placed on separate lines for code legibility:

```c
if ( true )
{
	// ...
}
```

- local project includes are always in double-quotes. separate libraries are referenced with brackets, along with standard headers. When practical, indicate the purpose when including headers.

```c
	#include <string> // for std::string
	#include "kernel.hpp" // for kernel instance
	#include <sdk/types.hpp>
```

- File extensions are .c / .h for C code. C code must support C++ compiles (guard in extern "C")
- File extensions for .cpp / .hpp for C++ code.
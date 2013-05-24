// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#pragma once

// This file should define the following macros for each compiler:
// PLATFORM_COMPILER - a string defining the compiler name
// PLATFORM_COMPILER_VERSION - a string specifying the compiler version
// PLATFORM_FANCY_FUNCTION - an alias to this compiler's pretty function macro

// setup compiler defines
#if defined( __GNUC__ )
	#define PLATFORM_COMPILER "gcc"
	#define PLATFORM_COMPILER_VERSION CONCAT_PERIOD( STRINGIZE(__GNUC__), STRINGIZE(__GNUC_MINOR__) )
	#define PLATFORM_FANCY_FUNCTION __PRETTY_FUNCTION__
#endif

#if defined( _MSC_VER )
	#define PLATFORM_COMPILER "msvc"
	#if _MSC_VER < 1300
		#define PLATFORM_COMPILER_VERSION STRINGIZE(12.0)
	#elif _MSC_VER == 1300
		#define PLATFORM_COMPILER_VERSION STRINGIZE(13.0)
	#elif _MSC_VER == 1310
		#define PLATFORM_COMPILER_VERSION STRINGIZE(13.1)
	#elif _MSC_VER == 1400
		#define PLATFORM_COMPILER_VERSION STRINGIZE(14.0)
	#elif _MSC_VER == 1500
		#define PLATFORM_COMPILER_VERSION STRINGIZE(15.0)
	#elif _MSC_VER == 1600
		#define PLATFORM_COMPILER_VERSION STRINGIZE(16.0)
	#else
		#define PLATFORM_COMPILER_VERSION "Unknown version of msvc"
	#endif
	#define PLATFORM_FANCY_FUNCTION __FUNCSIG__
#endif

// Define the following macros for this platform:
// PLATFORM_NAME - a string describing the OS at compile time (win32, linux, macosx)
#if _WIN32 || _WIN64
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN 1
	#endif

	#ifndef WIN32_EXTRA_LEAN
		#define WIN32_EXTRA_LEAN 1
	#endif

	// this is copied from Wine to prevent inclusion of unused definitions
	#define NOSERVICE
	#define NOMCX
	#define NOIME
	#define NOCOMM
	#define NOKANJI
	#define NORPC
	#define NOPROXYSTUB
	#define NOIMAGE
	#define NOTAPE

	#define PLATFORM_NAME "windows"
	#define PLATFORM_WINDOWS 1
#elif LINUX
	#if RASPBERRYPI
		#define PLATFORM_NAME "linux"
		#define PLATFORM_LINUX 1
	#else
		#define PLATFORM_NAME "raspberrypi"
		#define PLATFORM_LINUX 1
		#define PLATFORM_RASPBERRYPI 1
	#endif
#elif __APPLE__
	#include <TargetConditionals.h>

	#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		#define PLATFORM_NAME "ios"
		#define PLATFORM_IS_MOBILE 1
		#define PLATFORM_IOS 1
	#else
		#define PLATFORM_NAME "macosx"
		#define PLATFORM_MACOSX 1
	#endif
#elif __ANDROID__
	#define PLATFORM_NAME "Android"
	#define PLATFORM_ANDROID 1
	#define PLATFORM_IS_MOBILE 1
#else
	#error Unknown platform!
#endif

// disable some warnings
#if defined(_MSC_VER) // These are ignored by g++. Maybe they're ignored on other compilers?
	#pragma warning( disable: 4100 ) // : unreferenced formal parameter
	#pragma warning( disable: 4127 ) // : conditional expression is constant
	#pragma warning( disable: 4189 ) // : local variable is initialized but not referenced
	#pragma warning( disable: 4244 ) // : conversion from 'double' to 'float', possible loss of data
	#pragma warning( disable: 4267 ) // : conversion from size_t to int, possible loss of data
	#pragma warning( disable: 4305 ) // : truncate 'double' to 'float'
	#pragma warning( disable: 4312 ) // : 'operation' : conversion from 'type1' to 'type2' of greater size
	#pragma warning( disable: 4482 ) // : nonstandard extension used: enum 'PanelType_s::PanelType_e' used in qualified name
	#pragma warning( disable: 4530 ) // : C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	#pragma warning( disable: 4800 ) // : forcing value to bool 'true' or 'false' (performance warning)
	#pragma warning( disable: 4996 ) // : This function or variable may be unsafe. Consider using <alternative> instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
	

	//#pragma warning( disable: 4995 ) // : name was marked as #pragma deprecated
	// ^ Superceded by #define STRSAFE_NO_DEPRECATE on WIN32
#endif
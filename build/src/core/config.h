// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

// This file should define the following macros for each compiler:
// PLATFORM_COMPILER					a string defining the compiler name
// PLATFORM_COMPILER_VERSION 			a string specifying the compiler version
// PLATFORM_FANCY_FUNCTION 				an alias to this compiler's pretty function macro

// Bringing back these utility macros
#define STRINGIZE(x) STRINGIZE_THIS(x)
#define STRINGIZE_THIS(x) #x
#define CONCAT_PERIOD(x, y) x "." y

// setup compiler defines
#if defined( __clang__ )
	#define PLATFORM_COMPILER "clang"
	#define PLATFORM_COMPILER_VERSION CONCAT_PERIOD( STRINGIZE(__clang_major__), STRINGIZE(__clang_minor__))
	#define PLATFORM_FANCY_FUNCTION __PRETTY_FUNCTION__
#elif defined( __GNUC__ )
	#define PLATFORM_COMPILER "gcc"
	#define PLATFORM_COMPILER_VERSION CONCAT_PERIOD( STRINGIZE(__GNUC__), STRINGIZE(__GNUC_MINOR__) )
	#define PLATFORM_FANCY_FUNCTION __PRETTY_FUNCTION__
#elif defined( _MSC_VER )
	#define PLATFORM_COMPILER "msvc"
	#if _MSC_VER < 1300
		#define PLATFORM_COMPILER_VERSION STRINGIZE(12.0)
	#elif _MSC_VER == 1300
		#define PLATFORM_COMPILER_VERSION STRINGIZE(13.0) // Visual Studio 2002
	#elif _MSC_VER == 1310
		#define PLATFORM_COMPILER_VERSION STRINGIZE(13.1)
	#elif _MSC_VER == 1400
		#define PLATFORM_COMPILER_VERSION STRINGIZE(14.0) // Visual Studio 2005
	#elif _MSC_VER == 1500
		#define PLATFORM_COMPILER_VERSION STRINGIZE(15.0) // Visual Studio 2008
	#elif _MSC_VER == 1600
		#define PLATFORM_COMPILER_VERSION STRINGIZE(16.0) // Visual Studio 2010
	#elif _MSC_VER == 1700
		#define PLATFORM_COMPILER_VERSION STRINGIZE(17.0) // Visual Studio 2012
	#else
		#define PLATFORM_COMPILER_VERSION "Unknown version of msvc"
	#endif
	#define PLATFORM_FANCY_FUNCTION __FUNCSIG__
#else
	#error Unknown compiler!
#endif

// Define the following macros for each platform:
// PLATFORM_NAME 						A string describing the OS at compile time (win32, linux, macosx)
// PLATFORM_FILESYSTEM_SUPPORT			true if this platform can read/write to a physical filesystem

#if (defined(_WIN32) || defined(_WIN64)) && defined(_MSC_VER)
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
	#define NOMINMAX

	#define PLATFORM_NAME "windows"
	#define PLATFORM_WINDOWS 1
	#define PLATFORM_FILESYSTEM_SUPPORT 1
#elif defined(__ANDROID__) // needs to be above __linux__, since android defines it also.
	#define PLATFORM_NAME "android"
	#define PLATFORM_ANDROID 1
	#define PLATFORM_FILESYSTEM_SUPPORT 1
	#define PLATFORM_POSIX 1
#elif defined(__linux__)
	#if defined(PLATFORM_RASPBERRYPI)
		// specifically built for RaspberryPi
		#define PLATFORM_NAME "raspberrypi"
	#else
		// generic flavor
		#define PLATFORM_NAME "linux"
	#endif

	#define PLATFORM_LINUX 1
	#define PLATFORM_POSIX 1
	#define PLATFORM_FILESYSTEM_SUPPORT 1
#elif defined(__APPLE__)
	#include <TargetConditionals.h>

	#define PLATFORM_APPLE 1

	#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		#define PLATFORM_NAME "iphoneos"
		#define PLATFORM_IPHONEOS 1
		#define PLATFORM_FILESYSTEM_SUPPORT 0
	#else
		#define PLATFORM_NAME "macosx"
		#define PLATFORM_MACOSX 1
		#define PLATFORM_FILESYSTEM_SUPPORT 1
	#endif

	#define PLATFORM_POSIX 1
//	#define PLATFORM_BSD 1
#else
	#error Unknown platform!
#endif


#if !defined(PLATFORM_FILESYSTEM_SUPPORT)
	#error PLATFORM_FILESYSTEM_SUPPORT is not defined!
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

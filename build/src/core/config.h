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
	#define PLATFORM_THREAD_LOCAL __thread
	#define PLATFORM_ALIGN(x) __attribute__((aligned (x)))
#elif defined( __GNUC__ )
	#define PLATFORM_COMPILER "gcc"
	#define PLATFORM_COMPILER_VERSION CONCAT_PERIOD( STRINGIZE(__GNUC__), STRINGIZE(__GNUC_MINOR__) )
	#define PLATFORM_FANCY_FUNCTION __PRETTY_FUNCTION__
	#define PLATFORM_THREAD_LOCAL __thread
	#define PLATFORM_ALIGN(x) __attribute__((aligned (x)))
#elif defined( _MSC_VER )
	#define PLATFORM_COMPILER "msvc"
	#define PLATFORM_COMPILER_MSVC 1
	#define PLATFORM_THREAD_LOCAL __declspec(thread)
	#define PLATFORM_ALIGN(x) __declspec(align(x))
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
	#elif _MSC_VER == 1800
		#define PLATFORM_COMPILER_VERSION STRINGIZE(18.0) // Visual Studio 2013
	#elif _MSC_VER == 1900
		#define PLATFORM_COMPILER_VERSION STRINGIZE(19.0) // Visual Studio 2015
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

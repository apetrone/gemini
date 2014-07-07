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

#include "config.h"

#if PLATFORM_APPLE
	#include <TargetConditionals.h>
#endif

#if PLATFORM_WINDOWS
	#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
	#define MAX_PATH_SIZE MAX_PATH
	#define PATH_SEPARATOR '\\'
	#define PATH_SEPARATOR_STRING "\\"
#elif PLATFORM_LINUX || PLATFORM_APPLE || PLATFORM_ANDROID
	#include <limits.h>
	#define MAX_PATH_SIZE PATH_MAX
	#define PATH_SEPARATOR '/'
	#define PATH_SEPARATOR_STRING "/"
#else
	#error Unknown platform!
#endif

#include <stdio.h> // for size_t
#include "core.hpp" // for core::Error


namespace platform
{
	core::Error startup();
	void shutdown();
	
	// the directory where the active binary resides:
	// on Linux and Windows platforms, it returns the folder where the binary exists
	// on MacOS X when run as a command line tool, it returns the folder where the binary exists (similar to Linux and Windows)
	// on MacOS X / iPhoneOS (for Bundles), it returns the root bundle path (.app)
	core::Error program_directory( char * path, size_t size );

	namespace path
	{
		// normalize a path to the host platform's notation
		void normalize( char * path, size_t size );
		
		// make a directory at path
		core::Error make_directory( const char * path );
		
		// make all non-existent directories along a normalized_path
		void make_directories( const char * normalized_path );
	}; // namespace path
}; // namespace platform
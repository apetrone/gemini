#pragma once

#if __APPLE__
#include <TargetConditionals.h>
#endif

#if _WIN32
	#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
	#define MAX_PATH_SIZE MAX_PATH
	#define PATH_SEPARATOR '\\'
	#define PATH_SEPARATOR_STRING "\\"
	#elif LINUX || TARGET_OS_MAC
#include <limits.h>
	#define MAX_PATH_SIZE PATH_MAX
	#define PATH_SEPARATOR '/'
	#define PATH_SEPARATOR_STRING "/"
#endif


#include <stdio.h> // for size_t on .. (OSX?)

#include "core.hpp" // for core::Error


namespace platform
{
	core::Error startup();
	void shutdown();
	
	// the directory where the active binary resides:
	// on Linux and Windows platforms, it returns the folder where the binary exists
	// on MacOS X when run as a command line tool, it returns the folder where the binary exists (similar to Linux and Windows)
	// on MacOS X / iPhoneOS (for Bundles), it returns the root bundle path (.app)
	core::Error programDirectory( char * path, size_t size );

	namespace path
	{
		// normalize a path to the host platform's notation
		void normalize( char * path, size_t size );
		
		// make a directory at path
		core::Error makeDirectory( const char * path );
		
		// make all non-existent directories along a normalized_path
		void makeDirectories( const char * normalized_path );
	}; // namespace path
};
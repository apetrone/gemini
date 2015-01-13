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

#include <stdio.h> // for size_t

#include "config.h"

#if PLATFORM_APPLE
	#include <TargetConditionals.h>
#endif

#if PLATFORM_WINDOWS
	// see if we still need these here...
//	#define WIN32_LEAN_AND_MEAN 1
//	#include <windows.h>
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


#ifdef Success
	#undef Success
#endif

#ifdef Failure
	#undef Failure
#endif

#ifdef Warning
	#undef Warning
#endif

namespace platform
{
	struct Result
	{
		enum ResultStatus
		{
			Success,
			Failure = 0xBADDAE, 	// non-recoverable error, bad day :(
			Warning = 1 			// unexpected result, will proceed
		};
		
		ResultStatus status;
		const char* message;
		
		Result(ResultStatus result_status, const char* result_message = "") : status(result_status), message(result_message) {}
		bool failed() const { return status == Failure; }
		bool success() const { return status == Success; }
	};

	Result startup();
	void shutdown();

	namespace path
	{
		// normalize a path to the host platform's notation
		void normalize(char* path, size_t size);

		// make all non-existent directories along a normalized_path
		void make_directories(const char* normalized_path);
	} // namespace path
	
	struct DynamicLibrary {};
	typedef void* DynamicLibrarySymbol;
	
	struct TimerHandle {};
	
	
	struct DateTime
	{
		unsigned short year;
		unsigned short month; // 1-12
		unsigned short dayOfWeek; // 0-6, starting from Sunday
		unsigned short day;
		unsigned short hour; // 24-hour format
		unsigned short minute;
		unsigned short second;
		unsigned short milliseconds;
	};
	
	class IPlatformInterface
	{
	public:
		virtual ~IPlatformInterface() {}
		
		//
		// GENERAL
		//
		
		virtual Result startup() = 0;
		virtual void shutdown() = 0;
		
		//
		// PATHS
		//
		
		/// @desc Returns the directory where the active binary resides:
		/// on Linux and Windows platforms, it returns the folder where the binary exists
		/// on MacOS X when run as a command line tool, it returns the folder where the binary exists (similar to Linux and Windows)
		/// on MacOS X / iPhoneOS (for Bundles), it returns the root bundle path (.app)
		virtual Result get_program_directory(char* path, size_t size) = 0;
		
		/// @desc Make directory on disk
		virtual Result make_directory(const char* path) = 0;
		
		//
		// DYNAMIC LIBRARY HANDLING
		//
		
		/// @desc load a dynamic library at library_path
		/// @returns A pointer to a DynamicLibrary object on success; 0 on failure
		virtual DynamicLibrary* open_dynamiclibrary(const char* library_path) = 0;
		
		/// @desc close a library handle
		virtual void close_dynamiclibrary(DynamicLibrary* library) = 0;
		
		/// @desc Load a symbol from the dynamic library
		/// @returns 0 on failure, 1 on success
		virtual DynamicLibrarySymbol find_dynamiclibrary_symbol(DynamicLibrary* library, const char* symbol_name) = 0;
		
		//
		// TIMERS
		//
		
		/// @desc Create and return a handle to a platform timer.
		/// @returns Handle to a platform timer or 0 on failure.
		virtual TimerHandle* create_timer() = 0;
		
		/// @desc Destroys a previously created timer.
		virtual void destroy_timer(TimerHandle* timer) = 0;
		
		/// @desc Queries the timer for the current time
		/// @returns The current time as a double in miliseconds
		virtual double get_timer_msec(TimerHandle* timer) = 0;
		
		/// @desc Populates the DateTime struct with the system's current date and time
		virtual void get_current_datetime(DateTime& datetime) = 0;
	};

	IPlatformInterface* instance();
} // namespace platform
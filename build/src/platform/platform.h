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

#include <stdio.h> // for size_t

#include "config.h"

#if PLATFORM_APPLE
	#include <TargetConditionals.h>
#endif

#include <stdint.h>

#if PLATFORM_WINDOWS
	// see if we still need these here...
//	#define WIN32_LEAN_AND_MEAN 1
//	#include <windows.h>
	#define MAX_PATH_SIZE 260
	#define PATH_SEPARATOR '\\'
	#define PATH_SEPARATOR_STRING "\\"

	#define PLATFORM_LIBRARY_EXPORT __declspec(dllexport)
	#define PLATFORM_LIBRARY_IMPORT __declspec(dllimport)
#elif PLATFORM_LINUX || PLATFORM_APPLE || PLATFORM_ANDROID
	#include <limits.h>
	#define MAX_PATH_SIZE PATH_MAX
	#define PATH_SEPARATOR '/'
	#define PATH_SEPARATOR_STRING "/"

	#define PLATFORM_LIBRARY_EXPORT
	#define PLATFORM_LIBRARY_IMPORT
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

namespace kernel
{
	class IKernel;
}

namespace platform
{
#if PLATFORM_WINDOWS
	#define PLATFORM_MAIN int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR commandline, int show)
#elif PLATFORM_LINUX || PLATFORM_APPLE
	#define PLATFORM_MAIN int main(int argc, char** argv)
#elif PLATFORM_ANDROID
	// This requires: #include <android_native_app_glue.h>
	#define PLATFORM_MAIN void android_main(struct android_app* state)
#else
	#error Unknown platform!
#endif

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
	void parse_commandline(int argc, char** argv);
	int run_application(kernel::IKernel* instance);

	namespace path
	{
		// normalize a path to the host platform's notation
		void normalize(char* path);

		// make all non-existent directories along a normalized_path
		void make_directories(const char* normalized_path);
	} // namespace path
	
	struct DynamicLibrary
	{
		virtual ~DynamicLibrary() {}
	};

	typedef void* DynamicLibrarySymbol;

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
		
		/// @desc Returns the extension on this platform for a dynamiclibrary.
		/// @returns ".dylib", ".so", or ".dll" for Mac/Linux/Windows.
		/// NOTE: This MUST return the period character if required by the platform!
		virtual const char* get_dynamiclibrary_extension() const = 0;

		//
		// TIMERS
		//
		
		/// @desc Fetches the current time in microseconds
		/// @returns The current time in microseconds since the platform was
		/// instantiated.
		virtual uint64_t get_time_microseconds() = 0;
		
		/// @desc Create and return a handle to a platform timer.
		/// @returns Handle to a platform timer or 0 on failure.
//		virtual TimerHandle* create_timer() = 0;
		
		/// @desc Destroys a previously created timer.
//		virtual void destroy_timer(TimerHandle* timer) = 0;
		
		/// @desc Queries the timer for the current time
		/// @returns The current time as a double in miliseconds
//		virtual double get_timer_msec(TimerHandle* timer) = 0;
		
		/// @desc Populates the DateTime struct with the system's current date and time
		virtual void get_current_datetime(DateTime& datetime) = 0;
	};

	IPlatformInterface* instance();
} // namespace platform
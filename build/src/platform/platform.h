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

#include "config.h"



#if PLATFORM_APPLE
	#include <TargetConditionals.h>
#endif

#include <core/stackstring.h>

#include <stdint.h>
#include <stdio.h> // for size_t

#if PLATFORM_WINDOWS
	// see if we still need these here...
	#define WIN32_LEAN_AND_MEAN 1
	#define NOMINMAX
	#include <windows.h>
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

// http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine

namespace kernel
{
	class IKernel;
}




namespace platform
{
	enum ThreadStatus
	{
		THREAD_STATE_INACTIVE,
		THREAD_STATE_ACTIVE,
		THREAD_STATE_SUSPENDED
	};

	typedef void(*ThreadEntry)(void*);
} // namespace platform

// thread types

#if defined(PLATFORM_WINDOWS)
	#include "thread/windows/windows_thread.h"
#elif defined(PLATFORM_POSIX)
	#include "thread/posix/posix_thread.h"
#else
	#error Not implemented on this platform
#endif



namespace platform
{
#if PLATFORM_WINDOWS
	#define PLATFORM_MAIN int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR commandline, int show)
	struct MainParameters
	{
		char* commandline;
	};

#elif PLATFORM_LINUX || PLATFORM_APPLE
	#define PLATFORM_MAIN int main(int argc, char** argv)
	struct MainParameters
	{
		int argc;
		char** argv;
	};
#elif PLATFORM_ANDROID
	// This requires: #include <android_native_app_glue.h>
	#define PLATFORM_MAIN void android_main(struct android_app* state)
	
	// nothing since exec() is called on Android processes;
	// so they don't have command line arguments.
	struct MainParameters
	{
		// nothing
	};
#else
	#error Unknown platform!
#endif









	struct Thread
	{
		ThreadStatus state;
		ThreadId thread_id;
		ThreadHandle handle;
		ThreadEntry entry;
		void* userdata;
	};
	

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
	int run_application(kernel::IKernel* instance);
	void set_mainparameters(const MainParameters& params);
	const MainParameters& get_mainparameters();

	namespace path
	{
		// normalize a path to the host platform's notation
		void normalize(char* path);

		// make all non-existent directories along a normalized_path
		void make_directories(const char* normalized_path);
	} // namespace path
	
	struct DynamicLibrary
	{
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
	
	
	// ---------------------------------------------------------------------
	// dynamic library
	// ---------------------------------------------------------------------
	
	/// @desc load a dynamic library at library_path
	/// @returns A pointer to a DynamicLibrary object on success; 0 on failure
	DynamicLibrary* dylib_open(const char* library_path);
	
	/// @desc close a library handle
	void dylib_close(DynamicLibrary* library);
	
	/// @desc Tries to load a symbol from a dynamic library
	/// @returns A valid pointer to the symbol or null on failure
	DynamicLibrarySymbol dylib_find(DynamicLibrary* library, const char* symbol_name);
	
	/// @desc Returns the extension on this platform for a dynamiclibrary.
	/// @returns ".dylib", ".so", or ".dll" for Mac/Linux/Windows.
	/// NOTE: This MUST return the period character if required by the platform!
	const char* dylib_extension();
	
	// ---------------------------------------------------------------------
	// filesystem
	// ---------------------------------------------------------------------
	
	/// @desc Returns the directory where the active binary resides:
	/// on Linux and Windows platforms, it returns the folder where the binary exists
	/// on MacOS X when run as a command line tool, it returns the folder where the binary exists (similar to Linux and Windows)
	/// on MacOS X / iPhoneOS (for Bundles), it returns the root bundle path (.app)
	Result get_program_directory(char* path, size_t path_size);
	
	/// @desc Make directory on disk
	Result make_directory(const char* path);
	
	/// @desc Returns the value of the environment variable passed in
	/// or NULL, if it was not set
	const char* get_environment_variable(const char* name);
	
	/// @desc Returns the current user's directory;
	/// @returns The $(HOME) environment variable in Linux or %HOMEPATH% on Windows
	const char* get_user_directory();
	
	
	// this accepts a path entered by the user (possibly on the commandline)
	// and returns an expanded absolute path for use.
	// This should expand environment variables.
	// It should also account for leading tilde (~), which denotes the
	// special $(HOME) environment variable on Linux systems.
	core::StackString<MAX_PATH_SIZE> make_absolute_path(const char* path);
	

	// ---------------------------------------------------------------------
	// serial
	// ---------------------------------------------------------------------
	
	struct Serial
	{
	};
	
	Serial* serial_open(const char* device, uint32_t baud_rate);
	void serial_close(Serial* serial);
	
	/// @desc Read bytes to buffer from serial device
	/// @param total_bytes The maximum number of bytes to read into buffer
	/// @returns Total bytes read
	int serial_read(Serial* serial, void* buffer, int total_bytes);
	
	/// @desc Write bytes from buffer to serial device
	/// @param total_bytes The maximum number of bytes to write from the buffer
	/// @returns Total bytes read
	int serial_write(Serial* serial, const void* buffer, int total_bytes);
	
	// ---------------------------------------------------------------------
	// thread
	// ---------------------------------------------------------------------

	/// @desc Creates a thread with entry point
	Result thread_create(Thread& thread, ThreadEntry entry, void* data);
	
	/// @desc Should be called upon thread entry.
	///       This sets up signals, thread names, thread id and states.
	void thread_setup(void* data);
	
	/// @desc Wait for a thread to complete.
	/// @returns 0 on success; non-zero on failure (abnormal thread termination)
	int thread_join(Thread& thread);
	
	/// @desc Allows the calling thread to sleep
	void thread_sleep(int milliseconds);
	
	/// @desc
	void thread_detach(Thread& thread);
	
	
	/// @desc Get the calling thread's id
	/// @returns The calling thread's platform designated id
	ThreadId thread_id();
	
	
	
	void mutex_create();
	void mutex_destroy();
	void mutex_lock();
	void mutex_unlock();
	
	// ---------------------------------------------------------------------
	// time
	// ---------------------------------------------------------------------
	
	/// @desc Fetches the current time in microseconds
	/// @returns The current time in microseconds since the application started
	uint64_t microseconds();
	
	/// @desc Populates the DateTime struct with the system's current date and time
	void datetime(DateTime& datetime);


} // namespace platform
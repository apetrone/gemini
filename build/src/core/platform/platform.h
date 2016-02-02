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

#if defined(PLATFORM_APPLE)
	#include <TargetConditionals.h>
#endif

#include <core/array.h>
#include <core/stackstring.h>

#include <stdint.h>
#include <stdio.h> // for size_t

#if defined(PLATFORM_WINDOWS)
	// see if we still need these here...
	#define WIN32_LEAN_AND_MEAN 1
	#define NOMINMAX
	#include <windows.h>
	#define MAX_PATH_SIZE 260
	#define PATH_SEPARATOR '\\'
	#define PATH_SEPARATOR_STRING "\\"

	#define PLATFORM_LIBRARY_EXPORT __declspec(dllexport)
	#define PLATFORM_LIBRARY_IMPORT __declspec(dllimport)
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE) || defined(PLATFORM_ANDROID)
	#include <limits.h>
	#define MAX_PATH_SIZE PATH_MAX
	#define PATH_SEPARATOR '/'
	#define PATH_SEPARATOR_STRING "/"

	#define PLATFORM_LIBRARY_EXPORT
	#define PLATFORM_LIBRARY_IMPORT

	struct android_app;
#else
	#error Unknown platform!
#endif

// http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine

namespace kernel
{
	class IKernel;
	struct Parameters;
} // namespace kernel


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

// This section sets up three different platform abstraction macros.
// PLATFORM_RETURN					A platform specific return from main
// PLATFORM_MAIN					The function signature for this platform's application main
// PLATFORM_IMPLEMENT_PARAMETERS	A helper macro to setup application parameters

#if defined(PLATFORM_WINDOWS)
	struct MainParameters
	{
		char* commandline;

		LIBRARY_EXPORT MainParameters(char* commandline_string = nullptr) :
			commandline(commandline_string)
		{
		}
	};

	#define PLATFORM_RETURN(statement)\
		return statement

	#define PLATFORM_MAIN\
		 int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR commandline, int show)

	#define PLATFORM_IMPLEMENT_PARAMETERS()\
		platform::set_mainparameters(platform::MainParameters(commandline))

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
	struct MainParameters
	{
		int argc;
		char** argv;

		MainParameters(int total_arguments = 0, char** argument_params = nullptr) :
			argc(total_arguments),
			argv(argument_params)
		{
		}
	};

	#define PLATFORM_RETURN(statement)\
		return statement

	#define PLATFORM_MAIN\
		int main(int argc, char** argv)

	#define PLATFORM_IMPLEMENT_PARAMETERS()\
		platform::set_mainparameters(platform::MainParameters(argc, argv))

#elif defined(PLATFORM_ANDROID)
	struct MainParameters
	{
		struct android_app* app;

		MainParameters(struct android_app* state = 0) :
			app(state)
		{
		}
	};

	#define PLATFORM_RETURN(statement)\
		statement

	#define PLATFORM_MAIN\
		extern "C" void android_main(struct android_app* android_state)

	#define PLATFORM_IMPLEMENT_PARAMETERS()\
		platform::set_mainparameters(platform::MainParameters(android_state))
#else
	#error Unknown platform!
#endif

	enum ThreadStatus
	{
		THREAD_STATE_INACTIVE,
		THREAD_STATE_ACTIVE,
		THREAD_STATE_SUSPENDED
	};

	typedef void(*ThreadEntry)(void*);

	struct Thread
	{
		ThreadId thread_id;
		ThreadHandle handle;
		ThreadEntry entry;
		void* userdata;
		ThreadStatus state;
	};


	struct Result
	{
		const char* message;
		int status;

		LIBRARY_EXPORT Result(int result_status = 0, const char* result_message = "") :
			message(result_message),
			status(result_status)
		{
		}

		LIBRARY_EXPORT inline bool succeeded() const { return status == 0; }
		LIBRARY_EXPORT inline bool failed() const { return status != 0; }

		LIBRARY_EXPORT inline static Result success()
		{
			return Result(0);
		}

		LIBRARY_EXPORT inline static Result failure(const char* result_message)
		{
			return Result(-1, result_message);
		}

		LIBRARY_EXPORT inline static Result warning(const char* result_message)
		{
			return Result(1, result_message);
		}
	};


	/// @brief Starts low level system services. Calls core::memory::startup.
	LIBRARY_EXPORT Result startup();

	/// @brief Shutdown low level system services. Calls core::memory::shutdown.
	LIBRARY_EXPORT void shutdown();

	LIBRARY_EXPORT int run_application(kernel::IKernel* instance);
	LIBRARY_EXPORT void set_mainparameters(const MainParameters& params);
	LIBRARY_EXPORT const MainParameters& get_mainparameters();

	namespace path
	{
		// normalize a path to the host platform's notation
		LIBRARY_EXPORT void normalize(char* path);

		// make all non-existent directories along a normalized_path
		LIBRARY_EXPORT void make_directories(const char* normalized_path);
	} // namespace path

	struct DynamicLibrary
	{
	};

	typedef void* DynamicLibrarySymbol;
	typedef core::StackString<MAX_PATH_SIZE> PathString;

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

	struct File
	{
		void* handle;

		File() : handle(nullptr)
		{
		}

		bool is_open() const
		{
			return (handle != nullptr);
		}
	};

	enum FileMode
	{
		FileMode_Read,
		FileMode_Write
	};

	enum FileSeek
	{
		FileSeek_Begin,
		FileSeek_Relative,
		FileSeek_End
	};

	// ---------------------------------------------------------------------
	// dynamic library
	// ---------------------------------------------------------------------

	/// @brief load a dynamic library at library_path
	/// @returns A pointer to a DynamicLibrary object on success; 0 on failure
	LIBRARY_EXPORT DynamicLibrary* dylib_open(const char* library_path);

	/// @brief close a library handle
	LIBRARY_EXPORT void dylib_close(DynamicLibrary* library);

	/// @brief Tries to load a symbol from a dynamic library
	/// @returns A valid pointer to the symbol or null on failure
	LIBRARY_EXPORT DynamicLibrarySymbol dylib_find(DynamicLibrary* library, const char* symbol_name);

	/// @brief Returns the extension on this platform for a dynamiclibrary.
	/// @returns ".dylib", ".so", or ".dll" for Mac/Linux/Windows.
	/// NOTE: This MUST return the period character if required by the platform!
	LIBRARY_EXPORT const char* dylib_extension();

	// ---------------------------------------------------------------------
	// filesystem
	// ---------------------------------------------------------------------

	/// @brief Returns the directory where the active binary resides:
	/// on Linux and Windows platforms, it returns the folder where the binary exists
	/// on MacOS X when run as a command line tool, it returns the folder where the binary exists (similar to Linux and Windows)
	/// on MacOS X / iPhoneOS (for Bundles), it returns the root bundle path (.app)
	LIBRARY_EXPORT PathString get_program_directory();

	/// @brief Make directory on disk
	LIBRARY_EXPORT Result make_directory(const char* path);

	/// @brief Returns the value of the environment variable passed in
	/// or NULL, if it was not set.
	/// DO NOT include platform specific tokens: e.g. use 'HOME', not '$HOME'
	LIBRARY_EXPORT const char* get_environment_variable(const char* name);

	/// @brief Returns the current user's directory;
	/// @returns The $(HOME) environment variable in Linux or %HOMEPATH% on Windows
	LIBRARY_EXPORT PathString get_user_directory();

	// long-term storage for applications
	LIBRARY_EXPORT PathString get_user_application_directory(const char* application_data_path);

	// temporary storage; can be wiped by the OS
	LIBRARY_EXPORT PathString get_user_temp_directory();


	// this accepts a path entered by the user (possibly on the commandline)
	// and returns an expanded absolute path for use.
	// This should expand environment variables.
	// It should also account for leading tilde (~), which denotes the
	// special $(HOME) environment variable on Linux systems.
	LIBRARY_EXPORT core::StackString<MAX_PATH_SIZE> make_absolute_path(const char* path);


	LIBRARY_EXPORT platform::File fs_open(const char* path, FileMode mode = FileMode_Read);
	LIBRARY_EXPORT void fs_close(platform::File file);
	LIBRARY_EXPORT size_t fs_read(platform::File handle, void* destination, size_t size, size_t count);
	LIBRARY_EXPORT size_t fs_write(platform::File handle, const void* source, size_t size, size_t count);
	LIBRARY_EXPORT int32_t fs_seek(platform::File handle, long int offset, FileSeek origin);
	LIBRARY_EXPORT long int fs_tell(platform::File handle);
	LIBRARY_EXPORT bool fs_file_exists(const char* path);
	LIBRARY_EXPORT bool fs_directory_exists(const char* path);

	/// @brief Construct this platform's content directory
	/// @returns A string pointing to the absolute path for content on this platform
	/// example: On Mac/iOS/TVOS <AppBundle>/Content/Resources directory is the 'content' directory.
	LIBRARY_EXPORT PathString fs_content_directory();

	// ---------------------------------------------------------------------
	// process
	// ---------------------------------------------------------------------
	class Process
	{
	public:
		virtual ~Process();
	};

	LIBRARY_EXPORT Process* process_create(
		const char* executable_path,			// the path to the new process
		const Array<PathString>& arguments,		// arguments as strings
		const char* working_directory = nullptr	// startup working directory
	);

	LIBRARY_EXPORT void process_destroy(Process* process);
	LIBRARY_EXPORT bool process_is_running(Process* process);

	// ---------------------------------------------------------------------
	// serial
	// ---------------------------------------------------------------------

	struct Serial
	{
	};

	LIBRARY_EXPORT Serial* serial_open(const char* device, uint32_t baud_rate);
	LIBRARY_EXPORT void serial_close(Serial* serial);

	/// @brief Read bytes to buffer from serial device
	/// @param total_bytes The maximum number of bytes to read into buffer
	/// @returns Total bytes read
	LIBRARY_EXPORT int serial_read(Serial* serial, void* buffer, int total_bytes);

	/// @brief Write bytes from buffer to serial device
	/// @param total_bytes The maximum number of bytes to write from the buffer
	/// @returns Total bytes read
	LIBRARY_EXPORT int serial_write(Serial* serial, const void* buffer, int total_bytes);

	// ---------------------------------------------------------------------
	// system
	// ---------------------------------------------------------------------

	/// @brief Return the platform's pagesize in bytes
	LIBRARY_EXPORT size_t system_pagesize_bytes();

	/// @brief Return the number of physical CPUs in the system
	LIBRARY_EXPORT size_t system_processor_count();

	/// @brief Return the system's uptime (time since boot) in seconds
	LIBRARY_EXPORT double system_uptime_seconds();

	/// @brief Return a human readable version string for logging
	LIBRARY_EXPORT core::StackString<64> system_version_string();

	// ---------------------------------------------------------------------
	// thread
	// ---------------------------------------------------------------------

	/// @brief Creates a thread with entry point
	/// 	   This sets up signals, thread names, thread id and states.
	LIBRARY_EXPORT Result thread_create(Thread& thread, ThreadEntry entry, void* data);

	/// @brief Wait for a thread to complete.
	/// @returns 0 on success; non-zero on failure (abnormal thread termination)
	LIBRARY_EXPORT int thread_join(Thread& thread);

	/// @brief Allows the calling thread to sleep
	LIBRARY_EXPORT void thread_sleep(int milliseconds);

	/// @brief Detach the thread
	LIBRARY_EXPORT void thread_detach(Thread& thread);


	/// @brief Get the calling thread's id
	/// @returns The calling thread's platform designated id
	LIBRARY_EXPORT ThreadId thread_id();



	LIBRARY_EXPORT void mutex_create();
	LIBRARY_EXPORT void mutex_destroy();
	LIBRARY_EXPORT void mutex_lock();
	LIBRARY_EXPORT void mutex_unlock();

	// ---------------------------------------------------------------------
	// time
	// ---------------------------------------------------------------------

	/// @brief Fetches the current time in microseconds
	/// @returns The current time in microseconds since the application started
	LIBRARY_EXPORT uint64_t microseconds();

	/// @brief Fetch the tick count
	/// @returns The current tick count of the CPU of the calling thread
	LIBRARY_EXPORT uint64_t time_ticks();

	/// @brief Populates the DateTime struct with the system's current date and time
	LIBRARY_EXPORT void datetime(DateTime& datetime);


	// ---------------------------------------------------------------------
	// other platform stuff
	// ---------------------------------------------------------------------
	namespace OpenDialogFlags
	{
		constexpr uint8_t ShowHiddenFiles 		= 1;	// show hidden files
		constexpr uint8_t AllowMultiselect		= 2;	// allow multiple selection
		constexpr uint8_t CanCreateDirectories	= 4;	// directories can be created through this dialog
		constexpr uint8_t CanChooseDirectories	= 8;	// directories can be selected
		constexpr uint8_t CanChooseFiles		= 16;	// files can be selected
	} // namespace OpenDialogFlags

	LIBRARY_EXPORT Result show_open_dialog(const char* title, uint32_t open_flags, Array<PathString>& paths);

} // namespace platform

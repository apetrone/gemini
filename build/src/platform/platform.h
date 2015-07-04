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
	struct Parameters;
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
#if defined(PLATFORM_WINDOWS)
	#define PLATFORM_MAIN int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR commandline, int show)
	struct LIBRARY_EXPORT MainParameters
	{
		char* commandline;
	};

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
	#define PLATFORM_MAIN int main(int argc, char** argv)
	struct MainParameters
	{
		char** argv;
		int argc;
	};
#elif defined(PLATFORM_ANDROID)
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

	struct LIBRARY_EXPORT Thread
	{
		ThreadId thread_id;
		ThreadHandle handle;
		ThreadEntry entry;
		void* userdata;
		ThreadStatus state;
	};
	

	struct LIBRARY_EXPORT Result
	{
		enum ResultStatus
		{
			Success,
			Failure = 0xBADDAE, 	// non-recoverable error, bad day :(
			Warning = 1 			// unexpected result, will proceed
		};
		
		const char* message;
		ResultStatus status;
		
		Result(ResultStatus result_status, const char* result_message = "") : message(result_message), status(result_status) {}
		bool failed() const { return status == Failure; }
		bool success() const { return status == Success; }
	};
	


	LIBRARY_EXPORT Result startup();
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
	
	struct LIBRARY_EXPORT DynamicLibrary
	{
	};

	typedef void* DynamicLibrarySymbol;
	typedef core::StackString<MAX_PATH_SIZE> PathString;

	struct LIBRARY_EXPORT DateTime
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
	LIBRARY_EXPORT Result get_program_directory(char* path, size_t path_size);
	
	/// @brief Make directory on disk
	LIBRARY_EXPORT Result make_directory(const char* path);
	
	/// @brief Returns the value of the environment variable passed in
	/// or NULL, if it was not set
	LIBRARY_EXPORT const char* get_environment_variable(const char* name);
	
	/// @brief Returns the current user's directory;
	/// @returns The $(HOME) environment variable in Linux or %HOMEPATH% on Windows
	LIBRARY_EXPORT const char* get_user_directory();
	
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
	/// @param content_path The default content directory for bundled assets
	/// @param root_path The root path for this application (where the binary/executable resides)
	/// @returns platform::Result
	LIBRARY_EXPORT platform::Result fs_content_directory(core::StackString<MAX_PATH_SIZE>& content_path, const core::StackString<MAX_PATH_SIZE>& root_path);

	// ---------------------------------------------------------------------
	// serial
	// ---------------------------------------------------------------------
	
	struct LIBRARY_EXPORT Serial
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
	// thread
	// ---------------------------------------------------------------------

	/// @brief Creates a thread with entry point
	LIBRARY_EXPORT Result thread_create(Thread& thread, ThreadEntry entry, void* data);
	
	/// @brief Should be called upon thread entry.
	///       This sets up signals, thread names, thread id and states.
	LIBRARY_EXPORT void thread_setup(void* data);
	
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
	
	/// @brief Populates the DateTime struct with the system's current date and time
	LIBRARY_EXPORT void datetime(DateTime& datetime);


	// ---------------------------------------------------------------------
	// windowsystem
	// ---------------------------------------------------------------------
	
	struct WindowDimensions
	{
		uint32_t x;
		uint32_t y;
	
		// dimensions of the actual window in pixels
		uint32_t width;
		uint32_t height;
				
		// dimensions of the rendering area in pixels
		uint32_t render_width;
		uint32_t render_height;
		
		WindowDimensions() :
			x(0),
			y(0),
			width(1),
			height(1),
			render_width(0),
			render_height(1)
		{
		}
	};
	
	struct WindowParameters
	{
		WindowDimensions window;
		
		// in windowed modes, this is the target display the window
		// will be transferred to
		uint32_t target_display;

		// need to take this into account when calculating screen coordinates
		uint32_t titlebar_height;
		
		// utf8-encoded window title
		const char* window_title;
		
		// set to true to create a fullscreen window
		bool enable_fullscreen;
		
		// allow the window to be resized
		bool enable_resize;
		
		// wait for vertical sync
		bool enable_vsync;
		
		WindowParameters() :
		target_display(0),
		titlebar_height(0),
		window_title(0),
		enable_fullscreen(false),
		enable_resize(true),
		enable_vsync(true)
		{
		}
		
		virtual ~WindowParameters();
	};
	
	struct NativeWindow
	{
		virtual ~NativeWindow();
		
		NativeWindow(const WindowDimensions& window_dimensions) :
			dimensions(window_dimensions)
		{
		}
		
		WindowDimensions dimensions;
	};
		
	class input_provider
	{
	public:
		virtual ~input_provider();
	
		// capture the mouse
		virtual void capture_mouse(bool capture) = 0;
		
		// warp the mouse to a position
		virtual void warp_mouse(int x, int y) = 0;
		
		// get the current mouse position
		virtual void get_mouse(int& x, int& y) = 0;
		
		// toggle mouse visibility
		virtual void show_mouse(bool show) = 0;
	};
	

	NativeWindow* window_create(const WindowParameters& window_parameters);
	void window_destroy(NativeWindow* window);
	
	// activate this window for rendering
	void window_begin_rendering(NativeWindow* window);
	
	// post frame on this window
	void window_end_rendering(NativeWindow* window);
	
	// process window provider events (for all windows)
	void window_process_events();
	
	// return the window size in pixels
	void window_size(NativeWindow* window, int& width, int& height);
	
	// return the renderable window surface in pixels
	void window_render_size(NativeWindow* window, int& width, int& height);
	
	// total number of screens detected on this system
	size_t window_screen_count();
	
	/// @brief get the specified screen's rect (origin, width, and height) in pixels
	void window_screen_rect(size_t screen_index, int& x, int& y, int& width, int& height);
	
	// bring window to focus
	void window_focus(NativeWindow* window);
	
	// should these be exposed? or internal?
	enum RenderBackend
	{
		RenderBackend_None		= 0,
		RenderBackend_OpenGL	= 1,
		RenderBackend_OpenGLES	= 2
	};
	
	enum InputBackend
	{
		InputBackend_Cocoa,
		InputBackend_udev,
		InputBackend_win32
	};
	
	//LIBRARY_EXPORT bool supports_input_interface(const NativeInputInterface::Type& interface);

} // namespace platform

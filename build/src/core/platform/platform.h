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

#include <core/typedefs.h>

#include <core/array.h>
#include <core/stackstring.h>

#include <platform/input.h>

#include <stdint.h>
#include <stdio.h> // for size_t

#if defined(PLATFORM_WINDOWS)
	// see if we still need these here...
	#define WIN32_LEAN_AND_MEAN 1
	#define NOMINMAX
	#include <windows.h>
	#include <winsock2.h>
	#define MAX_PATH_SIZE 260
	#define PATH_SEPARATOR '\\'
	#define PATH_SEPARATOR_STRING "\\"

	namespace platform
	{
		typedef DWORD ThreadId;
	} // namespace platform

	#define GEMINI_EXPORT __declspec(dllexport)
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE) || defined(PLATFORM_ANDROID)
	#include <limits.h>
	#define MAX_PATH_SIZE PATH_MAX
	#define PATH_SEPARATOR '/'
	#define PATH_SEPARATOR_STRING "/"

	struct android_app;

	#include <pthread.h>
	namespace platform
	{
		typedef pthread_t ThreadId;
	} // namespace platform

	#include <netinet/in.h> // for sockaddr_in

	#define GEMINI_EXPORT
#else
	#error Unknown platform!
#endif

// http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine

namespace kernel
{
	class IKernel;
	struct Parameters;
} // namespace kernel

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

		MainParameters(char* commandline_string = nullptr) :
			commandline(commandline_string)
		{
		}
	};

	#define PLATFORM_RETURN(statement)\
		return statement

	#define PLATFORM_MAIN\
		 int WINAPI WinMain(HINSTANCE /*instance*/, HINSTANCE /*prev_instance*/, LPSTR commandline, int /*show*/)

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

	struct Result
	{
		const char* message;
		int status;

		Result(int result_status = 0, const char* result_message = "") :
			message(result_message),
			status(result_status)
		{
		}

		inline bool succeeded() const { return status == 0; }
		inline bool failed() const { return status != 0; }

		inline static Result success()
		{
			return Result(0);
		}

		inline static Result failure(const char* result_message)
		{
			return Result(-1, result_message);
		}

		inline static Result warning(const char* result_message)
		{
			return Result(1, result_message);
		}
	};


	/// @brief Starts low level system services. Calls core::memory::startup.
	Result startup();

	/// @brief Shutdown low level system services. Calls core::memory::shutdown.
	void shutdown();

	/// @brief Per frame platform update
	void update(float delta_milliseconds);

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
		virtual ~DynamicLibrary();
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
	DynamicLibrary* dylib_open(const char* library_path);

	/// @brief close a library handle
	void dylib_close(DynamicLibrary* library);

	/// @brief Tries to load a symbol from a dynamic library
	/// @returns A valid pointer to the symbol or null on failure
	DynamicLibrarySymbol dylib_find(DynamicLibrary* library, const char* symbol_name);

	/// @brief Returns the extension on this platform for a dynamiclibrary.
	/// @returns ".dylib", ".so", or ".dll" for Mac/Linux/Windows.
	/// NOTE: This MUST return the period character if required by the platform!
	const char* dylib_extension();

	// ---------------------------------------------------------------------
	// filesystem
	// ---------------------------------------------------------------------

	/// @brief Returns the directory where the active binary resides:
	/// on Linux and Windows platforms, it returns the folder where the binary exists
	/// on MacOS X when run as a command line tool, it returns the folder where the binary exists (similar to Linux and Windows)
	/// on MacOS X / iPhoneOS (for Bundles), it returns the root bundle path (.app)
	PathString get_program_directory();

	/// @brief Make directory on disk
	Result make_directory(const char* path);

	/// @brief Remove a directory from disk
	Result remove_directory(const char* path);

	/// @brief Returns the value of the environment variable passed in
	/// or NULL, if it was not set.
	/// DO NOT include platform specific tokens: e.g. use 'HOME', not '$HOME'
	const char* get_environment_variable(const char* name);

	/// @brief Returns the current user's directory;
	/// @returns The $(HOME) environment variable in Linux or %HOMEPATH% on Windows
	PathString get_user_directory();

	// long-term storage for applications
	PathString get_user_application_directory(const char* application_data_path);

	// temporary storage; can be wiped by the OS
	PathString get_user_temp_directory();


	// this accepts a path entered by the user (possibly on the commandline)
	// and returns an expanded absolute path for use.
	// This should expand environment variables.
	// It should also account for leading tilde (~), which denotes the
	// special $(HOME) environment variable on Linux systems.
	PathString make_absolute_path(const char* path);

	platform::File fs_open(const char* path, FileMode mode = FileMode_Read);
	void fs_close(platform::File file);
	size_t fs_read(platform::File handle, void* destination, size_t size, size_t count);
	size_t fs_write(platform::File handle, const void* source, size_t size, size_t count);
	int32_t fs_seek(platform::File handle, long int offset, FileSeek origin);
	long int fs_tell(platform::File handle);
	bool fs_file_exists(const char* path);
	bool fs_directory_exists(const char* path);

	/// @brief Construct this platform's content directory
	/// @returns A string pointing to the absolute path for content on this platform
	/// example: On Mac/iOS/TVOS <AppBundle>/Content/Resources directory is the 'content' directory.
	PathString fs_content_directory();

	// ---------------------------------------------------------------------
	// joystick
	// ---------------------------------------------------------------------

	// return the max number of joysticks supported on this platform
	uint32_t joystick_max_count();

	/// @brief Returns true if this joystick is physically connected.
	bool joystick_is_connected(uint32_t index);

	/// @brief Returns true if this joystick supports haptic feedback.
	bool joystick_supports_haptics(uint32_t index);

	/// @brief Set haptic force on joystick
	/// force: 0 being off, USHRT_MAX being full.
	void joystick_set_force(uint32_t index, uint16_t force);


	// ---------------------------------------------------------------------
	// network
	// ---------------------------------------------------------------------
#if defined(PLATFORM_WINDOWS)
	typedef SOCKET net_socket;
#elif defined(PLATFORM_POSIX)
	typedef int net_socket;
#else
	#error net_socket not defined for this platform!
#endif
	enum class net_socket_type
	{
		UDP,
		TCP
	};


	typedef struct sockaddr_in net_address;

	void net_address_init(net_address* address);
	void net_address_set(net_address* address, const char* ip, uint16_t port);
	int32_t net_address_host(net_address* address, char* buffer, size_t buffer_size);
	uint16_t net_address_port(net_address* address);
	void net_address_port(net_address* address, uint16_t port);

	void net_shutdown();

	// socket functions

	/// @returns 0 on success; -1 on failure
	net_socket net_socket_open(net_socket_type type);

	// @returns true if socket is valid, false otherwise.
	bool net_socket_is_valid(net_socket sock);

	void net_socket_close(net_socket sock);

	/// @returns 0 on success
	int32_t net_socket_bind(net_socket sock, net_address* interface);

	/// @brief Send data (TCP-only)
	/// @returns bytes written.
	int32_t net_socket_send(net_socket sock, const char* data, size_t data_size);

	/// @brief Send data (UDP-only)
/// @returns bytes read if > 0; otherwise an error code.
	int32_t net_socket_sendto(net_socket sock, net_address* destination, const char* data, size_t data_size);


	/// @brief Receive data (TCP-only)
	/// @returns bytes read if > 0; otherwise an error code.
	int32_t net_socket_recv(net_socket sock, char* buffer, size_t buffer_size);

	/// @brief Receive data (UDP-only)
	/// @returns bytes read if > 0; otherwise an error code.
	int32_t net_socket_recvfrom(net_socket sock, net_address* from, char* buffer, size_t buffer_size);

	// socket options

	// enable/disable address re-use
	int32_t net_socket_set_reuseaddr(net_socket sock, int32_t value);

	// enable/disable blocking i/o
	int32_t net_socket_set_blocking(net_socket sock, int32_t value);

	// returns 0 on success
	int32_t net_startup();

	// ---------------------------------------------------------------------
	// process
	// ---------------------------------------------------------------------
	class Process
	{
	public:
		virtual ~Process();
	};

	Process* process_create(
		const char* executable_path,			// the path to the new process
		const Array<PathString>& arguments,		// arguments as strings
		const char* working_directory = nullptr	// startup working directory
	);

	void process_destroy(Process* process);
	bool process_is_running(Process* process);

	// ---------------------------------------------------------------------
	// serial
	// ---------------------------------------------------------------------

	struct Serial
	{
		virtual ~Serial();
	};

	Serial* serial_open(const char* device, uint32_t baud_rate);
	void serial_close(Serial* serial);

	/// @brief Read bytes to buffer from serial device
	/// @param total_bytes The maximum number of bytes to read into buffer
	/// @returns Total bytes read
	int serial_read(Serial* serial, void* buffer, int total_bytes);

	/// @brief Write bytes from buffer to serial device
	/// @param total_bytes The maximum number of bytes to write from the buffer
	/// @returns Total bytes read
	int serial_write(Serial* serial, const void* buffer, int total_bytes);

	// ---------------------------------------------------------------------
	// system
	// ---------------------------------------------------------------------

	/// @brief Return the platform's pagesize in bytes
	size_t system_pagesize_bytes();

	/// @brief Return the number of physical CPUs in the system
	size_t system_processor_count();

	/// @brief Return the system's uptime (time since boot) in seconds
	double system_uptime_seconds();

	/// @brief Return a human readable version string for logging
	core::StackString<64> system_version_string();

	// ---------------------------------------------------------------------
	// thread
	// ---------------------------------------------------------------------
	enum ThreadStatus
	{
		THREAD_STATE_INACTIVE,
		THREAD_STATE_ACTIVE,
		THREAD_STATE_SUSPENDED
	};

	struct Thread
	{
		virtual ~Thread();

		void* user_data;
	};

	typedef void(*ThreadEntry)(Thread*);



	/// @brief Creates a thread with entry point
	/// This sets up signals, thread names, thread id and states.
	Thread* thread_create(ThreadEntry entry, void* data);

	/// @brief Destroys a thread created by thread_create
	void thread_destroy(Thread* thread);

	/// @brief Wait for a thread to complete.
	/// @param thread The target thread to wait on.
	/// @param timeout (Optional) Timeout to wait for the thread to finish in
	/// milliseconds. This is only applicable on Windows.
	/// If timeout is zero, this call blocks indefinitely.
	/// Otherwise, if the thread does not complete in timeout milliseconds,
	/// the thread is forcibly closed.
	/// The consequences of this behavior for each platform still remain.
	/// @returns 0 on success non-zero on failure (abnormal termination)
	int thread_join(Thread* thread, uint32_t timeout = 0);

	/// @brief Allows the calling thread to sleep
	void thread_sleep(int milliseconds);

	/// @brief Get the calling thread's id
	/// @returns The calling thread's platform designated id
	ThreadId thread_id();

	/// @brief Get the target thread's current status
	/// @returns ThreadStatus enum for thread.
	ThreadStatus thread_status(const Thread* thread);

	/// @brief Determine if the platform thread is still active.
	/// This will return false when the thread should terminate (determined
	/// by OS-specific mechanisms). Otherwise, work can continue like normal.
	/// If this returns false, your thread must exit shortly thereafter.
	/// This should only be called from created Threads.
	bool thread_is_active(Thread* thread);

	struct Mutex
	{
		virtual ~Mutex();
	};

	Mutex* mutex_create();
	void mutex_destroy(Mutex* mutex);
	void mutex_lock(Mutex* mutex);
	void mutex_unlock(Mutex* mutex);

	struct Semaphore
	{
		virtual ~Semaphore();
	};


	/// @brief Create a new semaphore
	Semaphore* semaphore_create(uint32_t initial_count, uint32_t max_count);

	/// @brief Wait indefinitely on a semaphore signal
	void semaphore_wait(Semaphore* sem);

	/// @brief Signal the semaphore
	void semaphore_signal(Semaphore* sem, uint32_t count = 1);

	/// @brief Destroy this semaphore
	void semaphore_destroy(Semaphore* sem);

	// ---------------------------------------------------------------------
	// time
	// ---------------------------------------------------------------------

	/// @brief Fetches the current time in microseconds
	/// @returns The current time in microseconds since the application started
	uint64_t microseconds();

	/// @brief Fetch the tick count
	/// @returns The current tick count of the CPU of the calling thread
	uint64_t time_ticks();

	/// @brief Populates the DateTime struct with the system's current date and time
	void datetime(DateTime& datetime);


	// ---------------------------------------------------------------------
	// other platform stuff
	// ---------------------------------------------------------------------
	struct PlatformExtensionDescription
	{
		const char* description;
		const char* extension;

		PlatformExtensionDescription(const char* desc = nullptr, const char* ext = nullptr)
			: description(desc)
			, extension(ext)
		{
		}
	};

	namespace OpenDialogFlags
	{
		constexpr uint8_t ShowHiddenFiles 		= 1;	// show hidden files
		constexpr uint8_t AllowMultiselect		= 2;	// allow multiple selection
		constexpr uint8_t CanCreateDirectories	= 4;	// directories can be created through this dialog
		constexpr uint8_t CanChooseDirectories	= 8;	// directories can be selected
		constexpr uint8_t CanChooseFiles		= 16;	// files can be selected
	} // namespace OpenDialogFlags

	Result show_open_dialog(const char* title, uint32_t open_flags, Array<PathString>& paths);

	namespace SaveDialogFlags
	{

	} // namespace SaveDialogFlags


	Result show_save_dialog(const char* title,
							uint32_t save_flags,
							const Array<PlatformExtensionDescription>& extensions,
							const PathString& default_extension,
							PathString& filename);

} // namespace platform

// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.
//
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
#include "platform.h"
#include "window.h"

#include <core/mem.h>

namespace platform
{
	core::memory::Zone* get_memory_zone();
	typedef core::memory::HeapAllocator<core::memory::DefaultTrackingPolicy> PlatformAllocatorType;
	PlatformAllocatorType& get_platform_allocator();

	// backend (low level platform interface)
	Result backend_startup();

	void backend_shutdown();

	void backend_update(float delta_milliseconds);

	// timer interface
	Result timer_startup();
	void timer_shutdown();

	// cross distro/system functions that could be shared

#if defined(PLATFORM_APPLE) || defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)

#if defined(PLATFORM_APPLE) || defined(PLATFORM_LINUX)
	int backend_run_application(int argc, const char** argv);
#elif defined(PLATFORM_ANDROID)
	int backend_run_application(struct android_app* app);
#endif

	// filesystem
	Result posix_make_directory(const char* path);
	Result posix_remove_directory(const char* path);
	const char* posix_get_user_directory();
	const char* posix_get_environment_variable(const char* name);
	core::StackString<MAX_PATH_SIZE> posix_make_absolute_path(const char* path);

	// dylib
	DynamicLibrary* posix_dylib_open(const char* library_path);
	void posix_dylib_close(DynamicLibrary* library);
	DynamicLibrarySymbol posix_dylib_find(DynamicLibrary* library, const char* symbol_name);

	// filesystem
	platform::File posix_fs_open(const char* path, FileMode mode);
	void posix_fs_close(platform::File file);
	size_t posix_fs_read(platform::File handle, void* destination, size_t size, size_t count);
	size_t posix_fs_write(platform::File handle, const void* source, size_t size, size_t count);
	int32_t posix_fs_seek(platform::File handle, long int offset, FileSeek origin);
	long int posix_fs_tell(platform::File handle);
	bool posix_fs_file_exists(const char* path);
	bool posix_fs_directory_exists(const char* path);

	// threads
	Thread* posix_thread_create(ThreadEntry entry, void* data);
	void posix_thread_destroy(Thread* thread);
	int posix_thread_join(Thread* thread);
	void posix_thread_sleep(int milliseconds);
	pthread_t posix_thread_id();
	ThreadStatus posix_thread_status(Thread* thread);
	bool posix_thread_is_active(Thread* thread);


	Semaphore* posix_semaphore_create(uint32_t initial_count, uint32_t max_count);
	void posix_semaphore_wait(Semaphore* sem);
	void posix_semaphore_signal(Semaphore* sem, uint32_t count);
	void posix_semaphore_destroy(Semaphore* sem);

	// time
	void posix_datetime(DateTime& datetime);
#elif defined(PLATFORM_WINDOWS)
	// windows is not fully posix compliant
#else
	#error Unknown platform!
#endif
} // namespace platform

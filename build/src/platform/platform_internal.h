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

namespace platform
{
	// timer interface
	Result timer_startup();
	void timer_shutdown();
	
	// os
	Result os_startup();
	int os_run_application(int argc, const char** argv);
	void os_shutdown();	

	// cross distro/system functions that could be shared
	
#if PLATFORM_APPLE || PLATFORM_LINUX

	// filesystem
	Result posix_make_directory(const char* path);
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
	Result posix_thread_create(Thread& thread, ThreadEntry entry, void* data);
	int posix_thread_join(Thread& thread);
	void posix_thread_sleep(int milliseconds);
	void posix_thread_detach(Thread& thread);
	ThreadId posix_thread_id();
	
	
	// time
	void posix_datetime(DateTime& datetime);
#endif
} // namespace platform

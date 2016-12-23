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
#include <platform/platform.h>

#include <core/stackstring.h>
#include <core/interface.h>
#include <core/array.h>

namespace core
{
	namespace filesystem
	{
		class IFileSystem
		{
		public:
			virtual ~IFileSystem();

			virtual void startup() = 0;
			virtual void shutdown() = 0;

			// general file system functions
			virtual bool file_exists(const char* path, bool path_is_relative = true) const = 0;
			virtual bool directory_exists(const char* path, bool path_is_relative = true) const = 0;

			// directory where the binary actually resides
			// this may vary between operating systems as some
			// will bundle their assets (MacOS X/iOS)
			virtual void root_directory(const ::platform::PathString& root_path) = 0;
			virtual const ::platform::PathString& root_directory() const = 0;

			virtual void content_directory(const ::platform::PathString& content) = 0;
			virtual const ::platform::PathString& content_directory() const = 0;

			// the user directory where this application's files are stored
			virtual const ::platform::PathString& user_application_directory() const = 0;
			virtual void user_application_directory(const ::platform::PathString& application_directory) = 0;

			// virtual file system functions
			virtual bool virtual_file_exists(const char* relative_path) const = 0;
			virtual bool virtual_directory_exists(const char* relative_path) const = 0;

			// Load a file into buffer. (synchronously)
			// bufferLength will contain the size of the buffer
			// if buffer is null, a new buffer is allocated and must be DEALLOC'd after use
			// if buffer is not null, bufferLength should contain the size of the buffer which will not be exceeded.
			virtual char* virtual_load_file(const char* relative_path, char* buffer, size_t* buffer_length) = 0;

			virtual void virtual_load_file(Array<unsigned char>& buffer, const char* relative_path) = 0;

			virtual void free_file_memory(void* memory) = 0;
		};


		GEMINI_DECLARE_INTERFACE(IFileSystem);


		// read an audio file to memory
		// this provides an abstraction between platforms; but likely needs to belong elsewhere?
		void* audiofile_to_buffer(const char* filename, size_t& buffer_length);

		void truncate_string_at_path(char* path, const char* substr);

//		int read_file_stats(const char* fullpath, FileStats& file_stats);


		void absolute_path_from_relative(::platform::PathString& fullpath, const char* relative_path, const ::platform::PathString& content_path);
		void relative_path_from_absolute(::platform::PathString& relative_path, const char* absolute_path, const ::platform::PathString& content_path);
	}




} // namespace core

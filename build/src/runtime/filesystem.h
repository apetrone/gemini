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

#include <platform/typedefs.h>
#include <platform/platform.h>

#include <core/stackstring.h>
#include <core/interface.h>

namespace platform
{
	struct File;

	enum FileMode;
}

namespace core
{
	namespace filesystem
	{
		class LIBRARY_EXPORT IFileSystem
		{
		public:
			virtual ~IFileSystem();
			
			virtual void startup() = 0;
			virtual void shutdown() = 0;

			virtual bool file_exists(const char* path, bool path_is_relative = true) const = 0;
			virtual bool directory_exists(const char* path, bool path_is_relative = true) const = 0;
			
			// directory where the binary actually resides
			// this may vary between operating systems as some
			// will bundle their assets (MacOS X/iOS)
			virtual void root_directory(const char* path) = 0;
			virtual const char* root_directory() const = 0;
			
			// the content directory is where resources for this application can be found
			virtual void content_directory(const char* path) = 0;
			virtual const char* content_directory() const = 0;
			
			virtual void absolute_path_from_relative(StackString<MAX_PATH_SIZE>& fullpath, const char* relative_path) const = 0;
			virtual void relative_path_from_absolute(StackString<MAX_PATH_SIZE>& relative_path, const char* absolute_path) const = 0;
		};
	
		// Load a file into buffer. The pointer is returned.
		// bufferLength will contain the size of the buffer
		// if buffer is null, a new buffer is allocated and must be DEALLOC'd after use
		// if buffer is not null, bufferLength should contain the size of the buffer which will not be exceeded.
		LIBRARY_EXPORT char* file_to_buffer(const char* filename, char* buffer, size_t* buffer_length, bool path_is_relative = true);
		
		// read an audio file to memory
		// this provides an abstraction between platforms; but likely needs to belong elsewhere?
		void* audiofile_to_buffer(const char* filename, size_t& buffer_length);
		
		LIBRARY_EXPORT void truncate_string_at_path(char* path, const char* substr);
		
//		LIBRARY_EXPORT int read_file_stats(const char* fullpath, FileStats& file_stats);
	}
	
	
	typedef Interface<filesystem::IFileSystem> fs;

} // namespace core
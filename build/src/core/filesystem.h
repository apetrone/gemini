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

#include <core/stackstring.h>

namespace core
{
	struct File
	{
		const char* create_buffer_from_file() { return 0; }
		void destroy_buffer(const char* buffer) { buffer = 0; }
	};

	// FileSystem class is used to implement platform-level logic

	class FileSystem
	{
	public:
		virtual ~FileSystem() {}
	
		virtual File* open(const char* path) = 0;
		virtual void close(File* file) = 0;
		
		virtual bool file_exists(const char* path) = 0;
	};
	
	namespace filesystem
	{
		struct FileStats
		{
			unsigned int last_modified_timestamp;
		};
	
		FileSystem* instance();
		
		void startup();
		void shutdown();
		
		// directory where the binary actually resides
		// this may vary between operating systems as some
		// will bundle their assets (MacOS X/iOS)
		void root_directory(char* path, int size);
		const char* root_directory();
		
		// the content directory is where resources for this application can be found
		void content_directory( const char* path );
		const char * content_directory();
		
		// return the platform content directory
		void construct_content_directory(StackString<MAX_PATH_SIZE>& path);
		
		// Load a file into buffer. The pointer is returned.
		// bufferLength will contain the size of the buffer
		// if buffer is null, a new buffer is allocated and must be DEALLOC'd after use
		// if buffer is not null, bufferLength should contain the size of the buffer which will not be exceeded.
		char * file_to_buffer(const char* filename, char* buffer, size_t* buffer_length, bool path_is_relative=true);
		
		// read an audio file to memory
		// this provides an abstraction between platforms; but likely needs to belong elsewhere?
		void * audiofile_to_buffer(const char* filename, size_t& buffer_length);
		
		// accepts path as a string with len: MAX_PATH_SIZE (as defined in platform.h)
		void absolute_path_from_relative(char* fullpath, const char* relativepath, const char* content_directory=0);
		void relative_path_from_absolute(char* relative_path, const char* absolute_path, const char* content_directory=0);
		
		void truncate_string_at_path(char* path, const char* substr);
		
		int read_file_stats(const char* fullpath, FileStats& file_stats);
		
		bool file_exists(const char* path, bool path_is_relative=true);
#if !PLATFORM_IS_MOBILE
		
		bool directory_exists(const char* path, bool path_is_relative=true);
#endif
		
#if PLATFORM_ANDROID
		void set_asset_manager(AAssetManager* asset_manager);
#endif
	}

} // namespace core
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
#include "filesystem_interface.h"

using namespace platform;

namespace core
{
	namespace filesystem
	{
		FileSystemInterface::~FileSystemInterface()
		{
		}

		void FileSystemInterface::startup()
		{
		}
		
		void FileSystemInterface::shutdown()
		{
			
		}
		
		bool FileSystemInterface::file_exists(const char* path, bool path_is_relative = true) const
		{
			PathString fullpath;
			if (path_is_relative)
			{
				absolute_path_from_relative(fullpath, path, content_path);
			}
			else
			{
				fullpath = path;
				fullpath.normalize(PATH_SEPARATOR);
			}
			
			return platform::fs_file_exists(fullpath());
		}
		
		bool FileSystemInterface::directory_exists(const char* path, bool path_is_relative = true) const
		{
			PathString fullpath;
			if (path_is_relative)
			{
				absolute_path_from_relative(fullpath, path, content_path);
			}
			else
			{
				fullpath = path;
				fullpath.normalize(PATH_SEPARATOR);
			}
			
			return platform::fs_directory_exists(fullpath());
		}
		
		void FileSystemInterface::root_directory(const ::platform::PathString& root)
		{
			root_path = root;
		}
		
		const ::platform::PathString& FileSystemInterface::root_directory() const
		{
			return root_path;
		}
		
		void FileSystemInterface::content_directory(const PathString& content_root)
		{
			content_path = content_root;
		}
		
		const PathString& FileSystemInterface::content_directory() const
		{
			return content_path;
		}
		
		const PathString& FileSystemInterface::user_application_directory() const
		{
			return user_application_path;
		}
		
		void FileSystemInterface::user_application_directory(const ::platform::PathString& application_directory)
		{
			user_application_path = application_directory;
		}
		
		bool FileSystemInterface::virtual_file_exists(const char* relative_path) const
		{
			return file_exists(relative_path, true);
		}
		
		bool FileSystemInterface::virtual_directory_exists(const char* relative_path) const
		{
			return directory_exists(relative_path, true);
		}
		
		char* FileSystemInterface::virtual_load_file(const char* relative_path, char* buffer, size_t* buffer_length) const
		{
			size_t file_size;
			
			if (!buffer_length)
			{
				fprintf(stderr, "ERROR: virtual_load_file called with INVALID value!\n");
				return 0;
			}
						
			StackString<MAX_PATH_SIZE> fullpath;
			absolute_path_from_relative(fullpath, relative_path, content_directory());
			if (!file_exists(fullpath(), false))
			{
				fprintf(stderr, "ERROR: File does not exist! \"%s\" (at \"%s\")\n", relative_path, fullpath());
				return 0;
			}

			platform::File handle = platform::fs_open(fullpath(), platform::FileMode_Read);
			if (handle.is_open())
			{
				// TODO: use 'get_length' to allow Android's Asset Manager?
				platform::fs_seek(handle, 0, platform::FileSeek_End);
				file_size = platform::fs_tell(handle);
				platform::fs_seek(handle, 0, platform::FileSeek_Begin);
				
				if (buffer && *buffer_length > 0)
				{
					if (file_size > *buffer_length)
					{
						fprintf(stderr, "Request to read file size larger than buffer! (%ld > %lu)\n", file_size, (unsigned long)*buffer_length);
						file_size = *buffer_length;
					}
				}
				
				*buffer_length = file_size;
				if (!buffer)
				{
					buffer = (char*)MEMORY_ALLOC((*buffer_length)+1, platform::memory::global_allocator());
					memset(buffer, 0, (*buffer_length)+1);
				}
				
				platform::fs_read(handle, buffer, 1, file_size);
				platform::fs_close(handle);
			}
			
			return buffer;
		}
	} // namespace filesystem
} // namespace core

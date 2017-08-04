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
#include "array.h"

#include <core/logging.h>
#include <core/mem.h>

namespace core
{
	namespace filesystem
	{
		// internal functions here
		void _internal_load_file(Array<unsigned char>& buffer, const char* absolute_path)
		{
			if (!platform::fs_file_exists(absolute_path))
			{
				LOGE("File does not exist! \"%s\" (at \"%s\")\n", absolute_path);
				return;
			}

			platform::File handle = platform::fs_open(absolute_path, platform::FileMode_Read);
			if (handle.is_open())
			{
				// TODO: use 'get_length' to allow Android's Asset Manager?
				size_t file_size;
				platform::fs_seek(handle, 0, platform::FileSeek_End);
				file_size = static_cast<size_t>(platform::fs_tell(handle));
				platform::fs_seek(handle, 0, platform::FileSeek_Begin);

				if (file_size > 0)
				{
					buffer.resize(file_size, 0);
				}

				platform::fs_read(handle, &buffer[0], 1, file_size);
				platform::fs_close(handle);
			}
		} // internal_load_file

		FileSystemInterface::FileSystemInterface()
		{
			allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_FILESYSTEM);
		}

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
			platform::PathString fullpath;
			if (path_is_relative)
			{
				absolute_path_from_relative(fullpath, path, content_path);
			}
			else
			{
				fullpath = path;
				fullpath.normalize(PATH_SEPARATOR);
			}

			LOGV("Checking path \"%s\"\n", fullpath());
			return platform::fs_file_exists(fullpath());
		}

		bool FileSystemInterface::directory_exists(const char* path, bool path_is_relative = true) const
		{
			platform::PathString fullpath;
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

		void FileSystemInterface::content_directory(const platform::PathString& content_root)
		{
			content_path = content_root;
		}

		const platform::PathString& FileSystemInterface::content_directory() const
		{
			return content_path;
		}

		const platform::PathString& FileSystemInterface::user_application_directory() const
		{
			return user_application_path;
		}

		void FileSystemInterface::user_application_directory(const ::platform::PathString& application_directory)
		{
			user_application_path = application_directory;
			if (!directory_exists(user_application_path(), false))
			{
				platform::make_directory(user_application_path());
			}
		}

		bool FileSystemInterface::virtual_file_exists(const char* relative_path) const
		{
			return file_exists(relative_path, true);
		}

		bool FileSystemInterface::virtual_directory_exists(const char* relative_path) const
		{
			return directory_exists(relative_path, true);
		}

		char* FileSystemInterface::virtual_load_file(const char* relative_path, char* buffer, size_t* buffer_length)
		{
			size_t file_size;

			if (!buffer_length)
			{
				LOGE("virtual_load_file called with INVALID value!\n");
				return 0;
			}

			StackString<MAX_PATH_SIZE> fullpath;
			absolute_path_from_relative(fullpath, relative_path, content_directory());
			if (!file_exists(fullpath(), false))
			{
				LOGE("File does not exist! \"%s\" (at \"%s\")\n", relative_path, fullpath());
				return 0;
			}

			platform::File handle = platform::fs_open(fullpath(), platform::FileMode_Read);
			if (handle.is_open())
			{
				// TODO: use 'get_length' to allow Android's Asset Manager?
				platform::fs_seek(handle, 0, platform::FileSeek_End);
				file_size = static_cast<size_t>(platform::fs_tell(handle));
				platform::fs_seek(handle, 0, platform::FileSeek_Begin);

				if (buffer && *buffer_length > 0)
				{
					if (file_size > *buffer_length)
					{
						LOGE("Request to read file size larger than buffer! (%lu > %lu)\n",
							(unsigned long)file_size,
							(unsigned long)*buffer_length
						);
						file_size = *buffer_length;
					}
				}

				*buffer_length = file_size;
				if (!buffer)
				{
					buffer = static_cast<char*>(MEMORY2_ALLOC(allocator, (*buffer_length) + 1));
					memset(buffer, 0, (*buffer_length)+1);
				}

				platform::fs_read(handle, buffer, 1, file_size);
				platform::fs_close(handle);
			}

			return buffer;
		} // virtual load file

		void FileSystemInterface::virtual_load_file(Array<unsigned char>& buffer, const char* relative_path)
		{
			platform::PathString fullpath;
			absolute_path_from_relative(fullpath, relative_path, content_directory());

			load_file(buffer, fullpath());		
		} // virtual_load_file

		void FileSystemInterface::free_file_memory(void* memory)
		{
			MEMORY2_DEALLOC(allocator, memory);
		} // free_file_memory

		void FileSystemInterface::load_file(Array<unsigned char>& buffer, const char* absolute_path)
		{
			_internal_load_file(buffer, absolute_path);
		} // load_file
	} // namespace filesystem
} // namespace core

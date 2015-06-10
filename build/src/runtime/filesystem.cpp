// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone
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
#include "filesystem.h"

#include <runtime/logging.h>

#include <platform/platform.h>
#include <platform/typedefs.h>

#include <core/str.h>

#include <stdio.h> // for printf
#include <sys/stat.h> // for fs::FileExists

namespace core
{
	namespace filesystem
	{
	
		IFileSystem::~IFileSystem()
		{
		}
	
#if PLATFORM_IS_MOBILE


		void * mobile_audio_file_to_buffer( const char * filename, size_t & buffer_length );
	
#endif

	} // namespace filesystem
} // namespace core



namespace core
{
	namespace filesystem
	{
		void truncate_string_at_path(char* path, const char* substr)
		{
			char* last;
			size_t len;
			size_t substr_len = str::len(substr);
			char buf[MAX_PATH_SIZE] = {0};
			len = str::len(path);
			last = path + len-1;
			for (int i = 0; path[i]; ++i)
			{
				char* p = strrchr(path, PATH_SEPARATOR);
				if (p)
				{
					memset(buf, 0, MAX_PATH_SIZE);
					str::copy(buf, path + len - (path+len-1-p), last-p);
					
					last = p-1;
					*p = '\0';
					
					if (str::case_insensitive_compare( buf, substr, substr_len ) == 0)
					{
						break;
					}
				}
			}
		} // truncate_string_at_path
		
		
		char* file_to_buffer(const char* filename, char* buffer, size_t* buffer_length, bool path_is_relative)
		{
			size_t file_size;
			
			if (!buffer_length)
			{
				LOGE("ERROR: file_to_buffer called with INVALID value!\n");
				return 0;
			}
			
			IFileSystem* fs = core::fs::instance();
			if (!fs->file_exists(filename, path_is_relative))
			{
				LOGE("File does not exist! \"%s\"\n", filename);
				return 0;
			}
			
			StackString<MAX_PATH_SIZE> fullpath;
			if ( path_is_relative )
			{
				fs->absolute_path_from_relative(fullpath, filename);
			}
			else
			{
				fullpath = filename;
				fullpath.normalize(PATH_SEPARATOR);
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
						fprintf(stdout, "Request to read file size larger than buffer! (%ld > %lu)\n", file_size, (unsigned long)*buffer_length);
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
		} // file_to_buffer

		void* audiofile_to_buffer(const char* filename, size_t& buffer_length)
		{
	#if PLATFORM_APPLE && PLATFORM_IS_MOBILE
			return mobile_audio_file_to_buffer(filename, buffer_length);
	#else
			return file_to_buffer(filename, 0, &buffer_length);
	#endif
		} // audiofile_to_buffer

//		int read_file_stats(const char* fullpath, FileStats& stats)
//		{
//	#if PLATFORM_LINUX || PLATFORM_ANDROID
//			struct stat file_stats;
//			stat(fullpath, &file_stats);
//
//		#if PLATFORM_LINUX
//				printf("time: %i\n", file_stats.st_mtim.tv_sec);
//		#else
//				printf("time: %i\n", file_stats.st_mtime);
//		#endif
//	#endif
//
//			return 0;
//		}

	} // namespace filesystem
} // namespace core
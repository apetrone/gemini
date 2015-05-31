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
#include "xfile.h"

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
#if PLATFORM_IS_MOBILE


		void * mobile_audio_file_to_buffer( const char * filename, size_t & buffer_length );
	
#endif

#if PLATFORM_ANDROID
	#include <android/asset_manager.h>

	static AAssetManager * _asset_manager = 0;
	void set_asset_manager( AAssetManager * asset_manager )
	{
		_asset_manager = asset_manager;
	}

#endif
	} // namespace filesystem
} // namespace core



namespace core
{
	namespace filesystem
	{
		static char _root_directory[MAX_PATH_SIZE];
		void root_directory(char* path, int size)
		{
			memset(_root_directory, 0, MAX_PATH_SIZE);
			str::copy(_root_directory, path, str::len(path));
		} // root_directory
		
		const char* root_directory()
		{
			return _root_directory;
		} // root_directory
		
		static char _content_directory[MAX_PATH_SIZE];
		void content_directory(const char* path)
		{
			memset(_content_directory, 0, MAX_PATH_SIZE);
			str::copy(_content_directory, path, str::len(path));
		} // content_directory

		const char* content_directory()
		{
			return _content_directory;
		} // content_directory

		void startup()
		{
			// TODO: create FileSystem instance?
		}
		
		void shutdown()
		{
			
		}

		void absolute_path_from_relative(char* fullpath, const char* relativepath, const char* content_directory)
		{
			if (!content_directory)
			{
				content_directory = filesystem::content_directory();
			}
			
			size_t path_size = str::len(content_directory);
			str::copy(fullpath, content_directory, path_size);
			str::cat(fullpath, PATH_SEPARATOR_STRING);
			str::cat(fullpath, relativepath);
			platform::path::normalize(fullpath);
		} // absolute_path_from_relative

		void relative_path_from_absolute(char* relative_path, const char* absolute_path, const char* content_directory)
		{
			if (!content_directory)
			{
				content_directory = filesystem::content_directory();
			}
			
			const char* temp = str::strstr(absolute_path, content_directory);

			size_t content_length = str::len(content_directory);
			if (temp)
			{
				str::copy(relative_path, (absolute_path+content_length+1), str::len(absolute_path) - content_length);
				platform::path::normalize(relative_path);
			}
		} // relative_path_from_absolute
		
		
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
		
		bool file_exists(const char* path, bool path_is_relative)
		{
			char fullpath[MAX_PATH_SIZE] = {0};
			int result = 0;
			
	#if PLATFORM_ANDROID
			// assume a relative path is always passed
			AAsset* asset = 0;
			asset = AAssetManager_open(_asset_manager, path, AASSET_MODE_BUFFER);
			if (asset)
			{
				AAsset_close(asset);
				return 1;
			}
			
			return 0;
	#else
			struct stat stFileInfo;
			if (path_is_relative)
			{
				filesystem::absolute_path_from_relative(fullpath, path);
			}
			else
			{
				str::copy(fullpath, path, 0);
				platform::path::normalize(fullpath);
			}
			result = stat(fullpath, &stFileInfo);
			return (result == 0) && ((stFileInfo.st_mode & S_IFMT) == S_IFREG);
	#endif
		} // file_exists
		
		bool directory_exists(const char* path, bool path_is_relative)
		{
			struct stat stFileInfo;
			int result = 0;
			char fullpath[MAX_PATH_SIZE] = {0};
			
			if (path_is_relative)
			{
				filesystem::absolute_path_from_relative(fullpath, path);
			}
			else
			{
				str::copy(fullpath, path, 0);
				platform::path::normalize(fullpath);
			}
			result = stat(fullpath, &stFileInfo);
			return (result == 0) && ((stFileInfo.st_mode & S_IFMT) == S_IFDIR);
		} // directory_exists
		
		void construct_content_directory(StackString<MAX_PATH_SIZE>& path)
		{
			path = root_directory();
#if PLATFORM_LINUX || PLATFORM_WINDOWS
	
#elif PLATFORM_MACOSX || PLATFORM_IPHONEOS
			// On Mac/iOS, the root directory points to the app bundle
			path.append(PATH_SEPARATOR_STRING);
			path.append("Resources");
			return;
#else
	#error Not yet implemented.
#endif
		}
		
		char* file_to_buffer(const char* filename, char* buffer, size_t* buffer_length, bool path_is_relative)
		{
			size_t file_size;
			
			if (!buffer_length)
			{
				LOGE("ERROR: file_to_buffer called with INVALID value!\n");
				return 0;
			}
			
			if (!filesystem::file_exists(filename, path_is_relative))
			{
				LOGE("File does not exist! \"%s\"\n", filename);
				return 0;
			}
			
	#if PLATFORM_ANDROID
			AAsset* asset = AAssetManager_open(_asset_manager, filename, AASSET_MODE_BUFFER);
			if ( asset )
			{
				file_size = AAsset_getLength(asset);
				
				if (buffer && *buffer_length > 0)
				{
					if (file_size > *buffer_length)
					{
						printf("Request to read file size larger than buffer! (%ld > %d)\n", file_size, *buffer_length);
						file_size = *buffer_length;
					}
				}
				
				*buffer_length = file_size;
				if (!buffer)
				{
					buffer = (char*)MEMORY_ALLOC((*buffer_length)+1, platform::memory::global_allocator());
					memset(buffer, 0, (*buffer_length)+1);
				}
				
				AAsset_read(asset, buffer, file_size);
				AAsset_close(asset);
			}

	#else
			char fullpath[MAX_PATH_SIZE] = {0};
			if ( path_is_relative )
			{
				filesystem::absolute_path_from_relative(fullpath, filename);
			}
			else
			{
				str::copy(fullpath, filename, 0);
				platform::path::normalize(fullpath);
			}

			xfile_t handle = xfile_open(fullpath, XF_READ);
			if (xfile_isopen(handle))
			{
				xfile_seek(handle, 0, XF_SEEK_END);
				file_size = xfile_tell(handle);
				xfile_seek(handle, 0, XF_SEEK_BEGIN);
				
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
				
				xfile_read(handle, buffer, 1, file_size);
				xfile_close(handle);
			}
	#endif
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

		int read_file_stats(const char* fullpath, FileStats& stats)
		{
	#if PLATFORM_LINUX || PLATFORM_ANDROID
			struct stat file_stats;
			stat(fullpath, &file_stats);

		#if PLATFORM_LINUX
				printf("time: %i\n", file_stats.st_mtim.tv_sec);
		#else
				printf("time: %i\n", file_stats.st_mtime);
		#endif
	#endif

			return 0;
		}

	} // namespace filesystem
} // namespace core
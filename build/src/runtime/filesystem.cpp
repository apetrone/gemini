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

#include <platform/platform.h>

#include <core/typedefs.h>
#include <core/logging.h>
#include <core/str.h>

#include <stdio.h> // for printf
#include <sys/stat.h> // for fs::FileExists

using platform::PathString;


namespace core
{
	namespace filesystem
	{

		IFileSystem::~IFileSystem()
		{
		}


		GEMINI_IMPLEMENT_INTERFACE(IFileSystem)

#if defined(PLATFORM_IPHONEOS)


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
					str::copy(buf,
						path + len - (path+len-1-p),
						static_cast<size_t>(last-p)+1);

					last = p-1;
					*p = '\0';

					if (str::case_insensitive_compare( buf, substr, substr_len ) == 0)
					{
						break;
					}
				}
			}
		} // truncate_string_at_path


		void absolute_path_from_relative(PathString& fullpath, const char* relative_path, const PathString& content_path)
		{
			fullpath = content_path;
			fullpath.append(PATH_SEPARATOR_STRING);
			fullpath.append(relative_path);
			fullpath.normalize(PATH_SEPARATOR);
		}

		void relative_path_from_absolute(PathString& relative_path, const char* absolute_path, const PathString& content_path)
		{
			const char* basepath = core::str::strstr(absolute_path, content_path());

			if (basepath)
			{
				size_t content_length = content_path.size();
				core::str::copy(&relative_path[0], (absolute_path+content_length+1), core::str::len(absolute_path) - content_length+1);
				relative_path.normalize(PATH_SEPARATOR);
			}
		}

		void* audiofile_to_buffer(const char* filename, size_t& buffer_length)
		{
	#if defined(PLATFORM_IPHONEOS)
			return mobile_audio_file_to_buffer(filename, buffer_length);
	#else
			return filesystem::instance()->virtual_load_file(filename, 0, &buffer_length);
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

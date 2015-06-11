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
#include "filesystem_interface.h"
//
//#include <runtime/logging.h>
//
//#include <platform/platform.h>
//#include <platform/typedefs.h>
//
//#include <core/str.h>
//
//#include <stdio.h> // for printf
//#include <sys/stat.h> // for fs::FileExists

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
			StackString<MAX_PATH_SIZE> fullpath;
			if (path_is_relative)
			{
				absolute_path_from_relative(fullpath, path);
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
			StackString<MAX_PATH_SIZE> fullpath;
			if (path_is_relative)
			{
				absolute_path_from_relative(fullpath, path);
			}
			else
			{
				fullpath = path;
				fullpath.normalize(PATH_SEPARATOR);
			}
			
			return platform::fs_directory_exists(fullpath());
		}
		
		void FileSystemInterface::root_directory(const char* path)
		{
			root_path = path;
		}
		
		const char* FileSystemInterface::root_directory() const
		{
			return root_path();
		}
		
		void FileSystemInterface::content_directory(const char* path)
		{
			content_path = path;
		}
		
		const char* FileSystemInterface::content_directory() const
		{
			return content_path();
		}
		
		void FileSystemInterface::absolute_path_from_relative(StackString<MAX_PATH_SIZE>& fullpath, const char* relative_path) const
		{
			fullpath = content_path;
			fullpath.append(PATH_SEPARATOR_STRING);
			fullpath.append(relative_path);
			fullpath.normalize(PATH_SEPARATOR);
		}
		
		void FileSystemInterface::relative_path_from_absolute(StackString<MAX_PATH_SIZE>& relative_path, const char* absolute_path) const
		{
			
		}
	} // namespace filesystem
} // namespace core
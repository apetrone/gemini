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

#include "platform_internal.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h> // for getenv

#include <core/stackstring.h>

namespace platform
{
	Result posix_make_directory(const char* path)
	{
		Result result;
		int status_code = 0;

		// http://pubs.opengroup.org/onlinepubs/009695399/functions/mkdir.html
		status_code = mkdir(path, (S_IRUSR | S_IWUSR | S_IXUSR ) | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH);
		if (status_code == -1)
		{
			// TODO: print out the errno
			result = Result::failure("mkdir failed!");
		}

		return result;
	} // posix_make_directory

	Result posix_remove_directory(const char* path)
	{
		Result result;
		int status_code = rmdir(path);
		assert(status_code == 0);
		return result;
	} // posix_remove_directory

	const char* posix_get_environment_variable(const char* name)
	{
		return getenv(name);
	} // posix_get_environment_variable

	const char* posix_get_user_directory()
	{
		return posix_get_environment_variable("HOME");
	} // posix_get_user_directory

	PathString posix_make_absolute_path(const char* path)
	{
		core::StackString<MAX_PATH_SIZE> output;
		if (path[0] == '~')
		{
			core::StackString<MAX_PATH_SIZE> input_path = path;
			output = platform::get_user_directory();
			output.append(input_path.substring(1, MAX_PATH_SIZE));
		}
		else
		{
			output = path;
		}

		return output;
	}

	platform::File posix_fs_open(const char* path, FileMode mode)
	{
		platform::File file;

		assert(mode == FileMode_Read || mode == FileMode_Write);

		const char* mode_flags;
		switch(mode)
		{
			case FileMode_Read: mode_flags = "rb"; break;
			case FileMode_Write: mode_flags = "wb"; break;
		}

		file.handle = fopen(path, mode_flags);
		return file;
	}

	void posix_fs_close(platform::File file)
	{
		fclose(static_cast<FILE*>(file.handle));
	}

	size_t posix_fs_read(platform::File file, void* destination, size_t size, size_t count)
	{
		size_t num_elements = fread(destination, size, count, static_cast<FILE*>(file.handle));

		// This assert is hit under these conditions:
		//	- size*count > file size
		assert(num_elements == count);
		return num_elements*size;
	}

	size_t posix_fs_write(platform::File file, const void* source, size_t size, size_t count)
	{
		size_t num_elements = fwrite(source, size, count, static_cast<FILE*>(file.handle));
		assert(count == num_elements);
		return num_elements*size;
	}

	int32_t posix_fs_seek(platform::File file, long int offset, FileSeek origin)
	{
		int internal_origin = 0;
		switch(origin)
		{
				// set position to absolute value of offset
			case FileSeek_Begin: internal_origin = SEEK_SET; break;

				// set position to current position + offset
			case FileSeek_Relative: internal_origin = SEEK_CUR; break;

				// set position to end of file
			case FileSeek_End: internal_origin = SEEK_END; break;
		}
		return fseek((FILE*)file.handle, offset, internal_origin);
	}

	long int posix_fs_tell(platform::File file)
	{
		return ftell(static_cast<FILE*>(file.handle));
	}

	bool posix_fs_file_exists(const char* path)
	{
		struct stat info;
		int result = stat(path, &info);
		return (result == 0) && ((info.st_mode & S_IFMT) == S_IFREG);
	}

	bool posix_fs_directory_exists(const char* path)
	{
		struct stat info;
		int result = stat(path, &info);
		return (result == 0) && ((info.st_mode & S_IFMT) == S_IFDIR);
	}
} // namespace platform

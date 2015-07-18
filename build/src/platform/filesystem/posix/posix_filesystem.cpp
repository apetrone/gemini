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

#include <string.h> // for strrchr

//#include <sys/sysinfo.h>
//#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h> // for snprintf
#include <stdlib.h> // for abort
#include <unistd.h> // for readlink, getpid

namespace platform
{
	PathString get_program_directory()
	{
		PathString output;
		int result = 0;
		{
			// http://www.flipcode.com/archives/Path_To_Executable_On_Linux.shtml
			char linkname[64] = {0};
			pid_t pid = getpid();
			
			if (snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid) < 0)
			{
				abort();
			}
			
			result = readlink(linkname, &output[0], output.max_size());
			if (result == -1)
			{
				PLATFORM_LOG(LogMessageType::Error, "read link failed!");
				return output;
			}
		}
		
		if (result != 0)
		{
			output = output.substring(0, result);

		}

		return output;
	}

	Result make_directory(const char* path)
	{
		return posix_make_directory(path);
	}

	const char* get_environment_variable(const char* name)
	{
		return posix_get_environment_variable(name);
	}

	PathString get_user_directory()
	{
		return posix_get_user_directory();
	}

	PathString get_user_application_directory(const char* application_data_path)
	{
		PathString user_application_directory = "/var/lib";
		user_application_directory.append(application_data_path);
		return user_application_directory;
	}

	PathString get_user_temp_directory()
	{
		// '/tmp' directory must be available for programs that require
		// temporary files. Programs must not assume that any files or
		// directories in /tmp are preserved between invocations of the program.
		// 
		// /var/tmp directory is made available for programs that require
		// temporary files or directories that are preserved between system
		// reboots. Therefore, this is more presistent than data in /tmp.
		return "/tmp";
	}

	core::StackString<MAX_PATH_SIZE> make_absolute_path(const char* path)
	{
		return posix_make_absolute_path(path);
	}

	platform::File fs_open(const char* path, FileMode mode)
	{
		return posix_fs_open(path, mode);
	}
	
	void fs_close(platform::File file)
	{
		return posix_fs_close(file);
	}
	
	size_t fs_read(platform::File file, void* destination, size_t size, size_t count)
	{
		return posix_fs_read(file, destination, size, count);
	}
	
	size_t fs_write(platform::File file, const void* source, size_t size, size_t count)
	{
		return posix_fs_write(file, source, size, count);
	}
	
	int32_t fs_seek(platform::File file, long int offset, FileSeek origin)
	{
		return posix_fs_seek(file, offset, origin);
	}
	
	long int fs_tell(platform::File file)
	{
		return posix_fs_tell(file);
	}
	
	bool fs_file_exists(const char* path)
	{
		return posix_fs_file_exists(path);
	}
	
	bool fs_directory_exists(const char* path)
	{
		return posix_fs_directory_exists(path);
	}

	PathString fs_content_directory()
	{
		return get_program_directory();
	}
} // namespace platform

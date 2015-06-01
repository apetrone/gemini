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
	Result get_program_directory(char* path, size_t path_size)
	{
		Result error(Result::Success);
		
		int result = 0;
		char* sep;
		{
			// http://www.flipcode.com/archives/Path_To_Executable_On_Linux.shtml
			char linkname[64] = {0};
			pid_t pid = getpid();
			
			if (snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid) < 0)
			{
				abort();
			}
			
			result = readlink(linkname, path, path_size);
			if (result == -1)
			{
				error.status = Result::Failure;
				error.message = "readlink failed";
			}
			else
			{
				path[result] = 0;
			}
		}
		
		if (result != 0)
		{
			sep = strrchr(path, PATH_SEPARATOR);
			
			if (sep)
			{
				*sep = '\0';
			}
		}
		
		return error;
	}

	Result make_directory(const char* path)
	{
		return posix_make_directory(path);
	}

	const char* get_user_directory()
	{
		return posix_get_user_directory();
	}

	core::StackString<MAX_PATH_SIZE> make_absolute_path(const char* path)
	{
		return posix_make_absolute_path(path);
	}
} // namespace platform

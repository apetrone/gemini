// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include "posix_platform_interface.h"

#include "posix/posix_dynamiclibrary.h"
#include "posix/posix_timer.h"
#include "posix/posix_filesystem.h"

using namespace platform;

#include <string.h> // for strrchr

#include <sys/sysinfo.h>
//#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h> // for snprintf
#include <stdlib.h> // for abort
#include <unistd.h> // for readlink, getpid

Result PosixPlatformInterface::startup()
{
	timer.reset();
	return Result(Result::Success);
}

void PosixPlatformInterface::shutdown()
{
	
}

Result PosixPlatformInterface::get_program_directory(char* path, size_t size)
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
		
		result = readlink(linkname, path, size);
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

Result PosixPlatformInterface::make_directory(const char* path)
{
	return posix_make_directory(path);
}

DynamicLibrary* PosixPlatformInterface::open_dynamiclibrary(const char* library_path)
{
	return 0;
}

void PosixPlatformInterface::close_dynamiclibrary(DynamicLibrary* library)
{
	
}

DynamicLibrarySymbol PosixPlatformInterface::find_dynamiclibrary_symbol(DynamicLibrary* library, const char* symbol_name)
{
	return 0;
}

const char* PosixPlatformInterface::get_dynamiclibrary_extension() const
{
	return ".so";
}

uint64_t PosixPlatformInterface::get_time_microseconds()
{
	return timer.get_microseconds();
}

void PosixPlatformInterface::get_current_datetime(DateTime& datetime)
{
	posix_get_date_time(datetime);
}

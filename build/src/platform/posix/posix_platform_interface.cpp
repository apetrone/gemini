// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#include "posix_platform_interface.h"

using namespace gemini::platform;

#include <sys/sysinfo.h>
//#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h> // for snprintf
#include <stdlib.h> // for abort
#include <unistd.h> // for readlink, getpid

Result PosixPlatformInterface::startup()
{
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

DynamicLibrary* PosixPlatformInterface::open_dynamic_library(const char* library_path)
{
	return 0;
}

void PosixPlatformInterface::close_dynamic_library(DynamicLibrary* library)
{
	
}

DynamicLibrarySymbol PosixPlatformInterface::find_dynamic_library_symbol(DynamicLibrary* library, const char* symbol_name)
{
	return 0;
}
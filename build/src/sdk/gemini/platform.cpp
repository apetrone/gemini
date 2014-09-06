// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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
#include <gemini/core.h>
#include <gemini/platform.h>
#include <gemini/platform_common.h>

#if TARGET_OS_MAC
	#include "platform/osx/osx_platform.h"
#endif

#if PLATFORM_WINDOWS
	#include <windows.h>
	#include <direct.h> // for _mkdir
#elif PLATFORM_LINUX
	#include <sys/sysinfo.h>
	//#include <errno.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <stdio.h> // for snprintf
	#include <stdlib.h> // for abort
	#include <unistd.h> // for readlink, getpid

#elif PLATFORM_APPLE
	#include <stdio.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <inttypes.h>
	#include <stdlib.h> // for realpath
	#include <unistd.h>	// for fork
#endif

#include <string.h> // for strrchr

using namespace core;

namespace platform
{
	core::Result startup()
	{
		Result result(Result::Success);
		
#if PLATFORM_APPLE
		result = osx_startup();
#endif

		
		return result;
	}
	
	void shutdown()
	{		
#if PLATFORM_APPLE
		osx_shutdown();
#endif
	}
	
	core::Result program_directory(char* path, size_t size)
	{
		Result error(Result::Success);
		int result = 0;
		char* sep;
		
#if PLATFORM_WINDOWS
		result = GetModuleFileNameA(GetModuleHandleA(0), path, size);
		if (result == 0)
		{
			error.status = core::Error::Failure;
			error.message = "GetModuleFileNameA failed!";
		}
		
#elif PLATFORM_LINUX
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
#endif
		
		if (result != 0)
		{
			sep = strrchr(path, PATH_SEPARATOR);
			
			if (sep)
			{
				*sep = '\0';
			}
		}
		
#if PLATFORM_APPLE
		error = osx_program_directory(path, size);
#endif
		return error;
	}
	
	
	namespace path
	{		
		Result make_directory(const char* path)
		{
			Result result(Result::Success);
			int status_code = 0;
			
#if PLATFORM_WINDOWS
			status_code = _mkdir(path);
			if (status_code == -1)
			{
				// TODO: print out the errno
				result = Result(Result::Failure, "_mkdir failed!");
			}
#elif PLATFORM_LINUX || PLATFORM_APPLE
			// http://pubs.opengroup.org/onlinepubs/009695399/functions/mkdir.html
			status_code = mkdir(path, (S_IRUSR | S_IWUSR | S_IXUSR ) | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH);
			if (status_code == -1)
			{
				// TODO: print out the errno
				result = Result(Result::Failure, "mkdir failed!");
			}
#endif
			
			return result;
		} // makeDirectory
	}; // namespace path
}; // namespace platform
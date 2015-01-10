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
#include "platform.h"
#include "platform_common.h"
#include "mem.h"
#include <assert.h>

#if TARGET_OS_MAC
	#include "osx/osx_platform.h"
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

#if PLATFORM_LINUX || PLATFORM_ANDROID
	#include "posix/posix_platform_interface.h"
	typedef PosixPlatformInterface PlatformInterface;
#elif PLATFORM_APPLE
	#include "osx/osx_platform_interface.h"
	typedef OSXPlatformInterface PlatformInterface;
#else
	#error Platform not implemented!
#endif


namespace gemini
{
	namespace platform
	{
		IPlatformInterface* _instance = 0;
		IPlatformInterface* instance()
		{
			return _instance;
		}
	
		Result startup()
		{
			Result result(Result::Success);
			_instance = CREATE(PlatformInterface);

			assert(_instance != 0);
			result = _instance->startup();
			
			return result;
		}
		
		void shutdown()
		{
			_instance->shutdown();
			DESTROY(IPlatformInterface, _instance);
		}
		
		Result program_directory(char* path, size_t size)
		{
	#if PLATFORM_WINDOWS
			Result error(Result::Success);
			int result = 0;
			char* sep;
			result = GetModuleFileNameA(GetModuleHandleA(0), path, size);
			if (result == 0)
			{
				error.status = platform::Result::Failure;
				error.message = "GetModuleFileNameA failed!";
			}
			
			if (result != 0)
			{
				sep = strrchr(path, PATH_SEPARATOR);
				
				if (sep)
				{
					*sep = '\0';
				}
			}
	#endif

			return _instance->get_program_directory(path, size);
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
			} // make_directory
		} // namespace path
	} // namespace platform
} // namespace gemini
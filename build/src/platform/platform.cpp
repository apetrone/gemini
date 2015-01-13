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
#elif PLATFORM_WINDOWS
	#include "windows/win32_platform_interface.h"
	typedef Win32PlatformInterface PlatformInterface;
#else
	#error Platform not implemented!
#endif

namespace platform
{
	IPlatformInterface* _instance = 0;
	IPlatformInterface* instance()
	{
		return _instance;
	}

	Result startup()
	{
		memory::startup();
		
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
		
		memory::shutdown();
	}
	
	Result program_directory(char* path, size_t size)
	{
		return _instance->get_program_directory(path, size);
	}
	
	namespace path
	{
		void normalize(char* path, size_t size)
		{
			while(*path)
			{
				if (*path == '/' || *path == '\\')
				{
					// conform to this platform's path separator
					*path = PATH_SEPARATOR;
				}
				
				++path;
			}
		} // normalize
		
		
		Result make_directory(const char* path)
		{
			return _instance->make_directory(path);
		} // make_directory
		

		void make_directories(const char* normalized_path)
		{
			const char* path = normalized_path;
			char directory[MAX_PATH_SIZE];
			
			// don't accept paths that are too short
			if (strlen(normalized_path) < 2)
			{
				return;
			}
			
			memset(directory, 0, MAX_PATH_SIZE);
			
			// loop through and call mkdir on each separate directory progressively
			while(*path)
			{
				if (*path == PATH_SEPARATOR)
				{
					strncpy(directory, normalized_path, (path+1)-normalized_path);
					platform::instance()->make_directory( directory );
				}
				
				++path;
			}
		} // make_directories
	} // namespace path
} // namespace platform
// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
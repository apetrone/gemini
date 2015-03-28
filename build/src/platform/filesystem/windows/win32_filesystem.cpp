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

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <windows.h>

#include <direct.h> // for _mkdir
#include <string.h> // for strrchr

namespace platform
{
	Result get_program_directory(char* path, size_t path_size)
	{
		Result error(Result::Success);
		
		int result = 0;
		char* sep;
		result = GetModuleFileNameA(GetModuleHandleA(0), path, path_size);
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
		return error;
	}

	Result make_directory(const char* path)
	{
		Result result(Result::Success);
		int status_code = 0;
		
		status_code = _mkdir(path);
		if (status_code == -1)
		{
			// TODO: print out the errno
			result = Result(Result::Failure, "_mkdir failed!");
		}
		
		return result;
	}
} // namespace platform
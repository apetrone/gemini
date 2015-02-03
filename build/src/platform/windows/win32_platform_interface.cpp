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

#include "mem.h"
#include "win32_platform_interface.h"

#include <direct.h> // for _mkdir
#include <string.h> // for strrchr

using namespace platform;

struct Win32DynamicLibrary : public DynamicLibrary
{
	HMODULE handle;
};


Result Win32PlatformInterface::startup()
{
	// prevent error dialogs from hanging the process.
	// these errors are forwarded to calling process.
	previous_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

	// cache off the frequency for later timer use.
	QueryPerformanceFrequency(&frequency);

	return Result(Result::Success);
}

void Win32PlatformInterface::shutdown()
{
	// restore the error mode
	SetErrorMode(previous_error_mode);
}

Result Win32PlatformInterface::get_program_directory(char* path, size_t size)
{
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
	return error;
}

Result Win32PlatformInterface::make_directory(const char* path)
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

DynamicLibrary* Win32PlatformInterface::open_dynamiclibrary(const char* library_path)
{
	Win32DynamicLibrary* library = CREATE(Win32DynamicLibrary);
	library->handle = LoadLibraryA(library_path);
	return library;
}

void Win32PlatformInterface::close_dynamiclibrary(DynamicLibrary* library)
{
	Win32DynamicLibrary* instance = static_cast<Win32DynamicLibrary*>(library);
	FreeLibrary(instance->handle);
	DESTROY(Win32DynamicLibrary, instance);
}

DynamicLibrarySymbol Win32PlatformInterface::find_dynamiclibrary_symbol(DynamicLibrary* library, const char* symbol_name)
{
	Win32DynamicLibrary* instance = static_cast<Win32DynamicLibrary*>(library);
	return GetProcAddress(instance->handle, (LPSTR)symbol_name);
}

const char* Win32PlatformInterface::get_dynamiclibrary_extension() const
{
	return ".dll";
}

// double Win32PlatformInterface::get_timer_msec(TimerHandle* timer)
// {
// 	Win32Timer* instance = static_cast<Win32Timer*>(timer);
// 	LARGE_INTEGER now;
// 	QueryPerformanceCounter(&now);
	

// 	return ((now.QuadPart - instance->last.QuadPart) / (double)_frequency.QuadPart) * 1000.0;
// }

uint64_t Win32PlatformInterface::get_time_microseconds()
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	return ((now.QuadPart) / (double)frequency.QuadPart) * 1000000;
}


void Win32PlatformInterface::get_current_datetime(DateTime& datetime)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	
	datetime.day = st.wDay;
	datetime.dayOfWeek = st.wDayOfWeek;
	datetime.hour = st.wHour;
	datetime.milliseconds = st.wMilliseconds;
	datetime.minute = st.wMinute;
	datetime.month = st.wMonth;
	datetime.second = st.wSecond;
	datetime.year = st.wYear;
}

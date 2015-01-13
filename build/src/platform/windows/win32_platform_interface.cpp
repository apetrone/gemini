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

#include "mem.h"
#include "win32_platform_interface.h"

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <direct.h> // for _mkdir
#include <string.h> // for strrchr

using namespace platform;


static LARGE_INTEGER _frequency;

struct Win32DynamicLibrary : public DynamicLibrary
{
	HMODULE handle;
};

struct Win32Timer : public TimerHandle
{
	LARGE_INTEGER last;
};

Result Win32PlatformInterface::startup()
{
	// prevent error dialogs from hanging the process.
	// these errors are forwarded to calling process.
	previous_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

	// cache off the frequency for later timer use.
	QueryPerformanceFrequency(&_frequency);

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

TimerHandle* Win32PlatformInterface::create_timer()
{
	Win32Timer* timer = CREATE(Win32Timer);
	get_timer_msec(timer);
	return timer;
}

void Win32PlatformInterface::destroy_timer(TimerHandle* timer)
{
	Win32Timer* instance = static_cast<Win32Timer*>(timer);
	DESTROY(Win32Timer, instance);
}

double Win32PlatformInterface::get_timer_msec(TimerHandle* timer)
{
	Win32Timer* instance = static_cast<Win32Timer*>(timer);
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	
	return ((now.QuadPart - instance->last.QuadPart) / (double)_frequency.QuadPart) * 1000.0;
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

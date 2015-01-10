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
#include "osx_platform_interface.h"
#include "osx_platform.h"

// we can use some posix-compatible functions
#include "posix/posix_dynamiclibrary.h"
#include "posix/posix_timer.h"

using namespace platform;

Result OSXPlatformInterface::startup()
{
	return osx_startup();
}

void OSXPlatformInterface::shutdown()
{
	osx_shutdown();
}

Result OSXPlatformInterface::get_program_directory(char* path, size_t size)
{
	return osx_program_directory(path, size);
}

Result OSXPlatformInterface::make_directory(const char* path)
{
	return Result(Result::Success);
}

DynamicLibrary* OSXPlatformInterface::open_dynamiclibrary(const char* library_path)
{
	return posix_dynamiclibrary_open(library_path);
}

void OSXPlatformInterface::close_dynamiclibrary(DynamicLibrary* library)
{
	posix_dynamiclibrary_close(library);
}

DynamicLibrarySymbol OSXPlatformInterface::find_dynamiclibrary_symbol(DynamicLibrary* library, const char* symbol_name)
{
	return posix_dynamiclibrary_find(library, symbol_name);
}


TimerHandle* OSXPlatformInterface::create_timer()
{
	return posix_create_timer();
}

void OSXPlatformInterface::destroy_timer(TimerHandle* timer)
{
	return posix_destroy_timer(timer);
}

double OSXPlatformInterface::get_timer_msec(TimerHandle* timer)
{
	return posix_get_timer_msec(timer);
}

void OSXPlatformInterface::get_current_datetime(DateTime& datetime)
{
	return posix_get_date_time(datetime);
}
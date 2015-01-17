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
#include "osx_platform_interface.h"
#include "osx_platform.h"

// we can use some posix-compatible functions
#include "posix/posix_dynamiclibrary.h"
#include "posix/posix_timer.h"
#include "posix/posix_filesystem.h"

#include <TargetConditionals.h>

#if TARGET_OS_MAC
	#include <CoreGraphics/CoreGraphics.h>
#else
	#error Not implemented on this platform
#endif

using namespace platform;

Result OSXPlatformInterface::startup()
{
	return osx_startup();
}

void OSXPlatformInterface::shutdown()
{
	osx_shutdown();
}

void OSXPlatformInterface::set_cursor_position(int x, int y)
{
	CGPoint target = CGPointMake(x, y);
//	NSScreen* screen = [NSScreen screen];
	
	// this operates in the global display coordinate space
	CGWarpMouseCursorPosition(target);
	CGAssociateMouseAndMouseCursorPosition(true);
}

Result OSXPlatformInterface::get_program_directory(char* path, size_t size)
{
	return osx_program_directory(path, size);
}

Result OSXPlatformInterface::make_directory(const char* path)
{
	return posix_make_directory(path);
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
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
#pragma once

#include "platform.h"

using platform::Result;
using platform::IPlatformInterface;
using platform::DynamicLibrary;
using platform::DynamicLibrarySymbol;
using platform::TimerHandle;
using platform::DateTime;

class Win32PlatformInterface : public IPlatformInterface
{
	unsigned int previous_error_mode;

public:
	virtual Result startup();
	virtual void shutdown();

	virtual void set_cursor_position(int x, int y);

	virtual Result get_program_directory(char* path, size_t size);
	virtual Result make_directory(const char* path);

	virtual DynamicLibrary* open_dynamiclibrary(const char* library_path);
	virtual void close_dynamiclibrary(DynamicLibrary* library);
	virtual DynamicLibrarySymbol find_dynamiclibrary_symbol(DynamicLibrary* library, const char* symbol_name);
	virtual const char* get_dynamiclibrary_extension() const;

	virtual TimerHandle* create_timer();
	virtual void destroy_timer(TimerHandle* timer);
	virtual double get_timer_msec(TimerHandle* timer);
	virtual void get_current_datetime(DateTime& datetime);
};

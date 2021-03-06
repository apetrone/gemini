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

#include "mem.h"
#include "platform_internal.h"

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <windows.h>

namespace platform
{
	struct Win32DynamicLibrary : public DynamicLibrary
	{
		HMODULE handle;
	};

	DynamicLibrary* dylib_open(const char* library_path)
	{
		Win32DynamicLibrary* library = MEMORY2_NEW(get_platform_allocator2(), Win32DynamicLibrary);
		library->handle = LoadLibraryA(library_path);
		return library;
	}

	void dylib_close(DynamicLibrary* library)
	{
		Win32DynamicLibrary* instance = static_cast<Win32DynamicLibrary*>(library);
		FreeLibrary(instance->handle);
		MEMORY2_DELETE(get_platform_allocator2(), instance);
	}

	DynamicLibrarySymbol dylib_find(DynamicLibrary* library, const char* symbol_name)
	{
		Win32DynamicLibrary* instance = static_cast<Win32DynamicLibrary*>(library);
		return GetProcAddress(instance->handle, (LPSTR)symbol_name);
	}

	const char* dylib_extension()
	{
		return ".dll";
	}
} // namespace platform

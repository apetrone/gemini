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

#include <core/logging.h>

#include <dlfcn.h>

namespace platform
{
	struct PosixDynamicLibrary : public DynamicLibrary
	{
		void* handle;
	};

	DynamicLibrary* posix_dylib_open(const char* library_path)
	{
		void* handle = dlopen(library_path, RTLD_LAZY);
		if (!handle)
		{
			LOGE("dlopen failed: %s", dlerror());
			return 0;
		}

		PosixDynamicLibrary* lib = MEMORY_NEW(PosixDynamicLibrary, get_platform_allocator());
		lib->handle = handle;
		return lib;
	}

	void posix_dylib_close(DynamicLibrary* library)
	{
		PosixDynamicLibrary* lib = static_cast<PosixDynamicLibrary*>(library);
		dlclose(lib->handle);

		MEMORY_DELETE(lib, get_platform_allocator());
	}

	DynamicLibrarySymbol posix_dylib_find(DynamicLibrary* library, const char* symbol_name)
	{
		PosixDynamicLibrary* lib = static_cast<PosixDynamicLibrary*>(library);
		return dlsym(lib->handle, symbol_name);
	}

} // namespace platform
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
#include "posix_dynamiclibrary.h"

#include <dlfcn.h>

namespace gemini
{
	namespace platform
	{
		struct PosixDynamicLibrary : public DynamicLibrary
		{
			void* handle;
		};
	
		DynamicLibrary* posix_dynamiclibrary_open(const char* library_path)
		{
			void* handle = dlopen(library_path, RTLD_LAZY);
			if (!handle)
			{
				return 0;
			}
			
			PosixDynamicLibrary* lib = CREATE(PosixDynamicLibrary);
			lib->handle = handle;
			return lib;
		}
		
		void posix_dynamiclibrary_close(DynamicLibrary* library)
		{
			PosixDynamicLibrary* lib = static_cast<PosixDynamicLibrary*>(library);
			dlclose(lib->handle);
			
			DESTROY(PosixDynamicLibrary, lib);
		}
		
		DynamicLibrarySymbol posix_dynamiclibrary_find(DynamicLibrary* library, const char* symbol_name)
		{
			PosixDynamicLibrary* lib = static_cast<PosixDynamicLibrary*>(library);
			return dlsym(lib->handle, symbol_name);
		}
	} // namespace platform
} // namespace gemini

// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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
#pragma once

#include "platform.h"

using platform::Result;
using platform::IPlatformInterface;
using platform::DynamicLibrary;
using platform::DynamicLibrarySymbol;

class OSXPlatformInterface : public IPlatformInterface
{
public:
	virtual Result startup();
	virtual void shutdown();
	
	virtual Result get_program_directory(char* path, size_t size);
	virtual Result make_directory(const char* path);
	
	virtual DynamicLibrary* open_dynamiclibrary(const char* library_path);
	virtual void close_dynamiclibrary(DynamicLibrary* library);
	virtual DynamicLibrarySymbol find_dynamiclibrary_symbol(DynamicLibrary* library, const char* symbol_name);
};
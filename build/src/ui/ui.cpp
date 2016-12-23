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
#include "ui/ui.h"
#include "ui/utils.h"

#include <stdlib.h>

namespace gui
{
	void* default_gui_malloc(size_t bytes)
	{
		return malloc(bytes);
	} // default_gui_malloc

	void default_gui_free(void* pointer)
	{
		free(pointer);
	} // default_gui_free

	gui_malloc _gmalloc = default_gui_malloc;
	gui_free _gfree = default_gui_free;

	gemini::Allocator* _allocator = nullptr;

	void set_allocator(gui_malloc malloc_fn, gui_free free_fn)
	{
		_gmalloc = malloc_fn;
		_gfree = free_fn;
	}

	void set_allocator(gemini::Allocator& allocator)
	{
		_allocator = &allocator;
	}

	gemini::Allocator& gui_allocator()
	{
		// If you hit this, the allocator hasn't been set.
		assert(_allocator);
		return *_allocator;
	}

	ResourceCache::~ResourceCache()
	{
	}
} // namespace gui

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
#if 0
USAGE:

#endif

#pragma once

#include <stdio.h>
#include <string>
#include <stdint.h>

#include <core/typedefs.h>

namespace gui
{	
	typedef void* (*gui_malloc)(size_t bytes);
	typedef void (*gui_free)(void* ptr);
	
	extern gui_malloc _gmalloc;
	extern gui_free _gfree;
	
	void LIBRARY_EXPORT set_allocator(gui_malloc malloc_fn, gui_free free_fn);
	
	typedef float real;


	struct TextureResource {};
	struct FontResource {};


	template <class T>
	struct ResourceHandle
	{
		int ref;

		ResourceHandle(int reference = -1) :
			ref(reference)
		{
		}

		bool is_valid() const
		{
			return (ref != -1);
		}

		operator int() const
		{
			return ref;
		}
	};


	typedef ResourceHandle<TextureResource> TextureHandle;
	typedef ResourceHandle<FontResource> FontHandle;

	enum TextureResult
	{
		TextureResult_Success = 0,
		TextureResult_Failed = 1,
	}; // TextureResult

	enum FontResult
	{
		FontResult_Success = 0,
		FontResult_Failed = 1,
	}; // FontResult

	class ResourceCache
	{
	public:
		virtual ~ResourceCache();

		virtual FontHandle create_font(const char* filename, size_t pixel_size) = 0;

		// destroy a font
		virtual void destroy_font(const FontHandle& handle) = 0;

		// return the texture used by this font
		virtual TextureHandle texture_for_font(const FontHandle& handle) = 0;

		virtual TextureHandle create_texture(const char* filename) = 0;
		virtual void destroy_texture(const TextureHandle& handle) = 0;
	};
}

#include "ui/utils.h"
#include "ui/events.h"
#include "ui/renderer.h"

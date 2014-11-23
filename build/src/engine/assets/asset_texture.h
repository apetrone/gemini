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

#include <core/stackstring.h>



#include "assets.h"

#include <renderer/renderer.h>
#include <renderer/image.h>

namespace assets
{
	// -------------------------------------------------------------
	// Texture
	struct Texture : public Asset
	{
		char * path;
		image::Image image;
		renderer::Texture* texture;
		
		Texture();
		virtual ~Texture();
		virtual void release();
	};
	
	
	struct TextureParameters : public AssetParameters
	{
		unsigned int flags;
		TextureParameters() : flags(0) {}
	};
		
//	Texture * load_cubemap( const char * basename, unsigned int flags = 0, bool ignore_cache = false );
	renderer::Texture* load_texture_from_file(const char * filename, unsigned int flags, image::Image& image);
	
	AssetLoadStatus texture_load_callback( const char * path, Texture * texture, const TextureParameters & parameters );
	void texture_construct_extension( StackString<MAX_PATH_SIZE> & extension );

	DECLARE_ASSET_LIBRARY_ACCESSOR(Texture, TextureParameters, textures);
}; // namespace assets
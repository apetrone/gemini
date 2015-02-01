// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#pragma once

#include <core/stackstring.h>



#include "assets.h"

#include <renderer/renderer.h>
#include <renderer/image.h>

namespace gemini
{
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
		void texture_construct_extension( core::StackString<MAX_PATH_SIZE> & extension );

		DECLARE_ASSET_LIBRARY_ACCESSOR(Texture, TextureParameters, textures);
	}; // namespace assets
} // namespace gemini
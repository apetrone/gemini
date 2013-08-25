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

#include "assets.hpp"
#include "stackstring.hpp"
#include "renderer.hpp"

namespace assets
{
	struct SpriteFrame
	{
		renderer::UV texcoords[4];
	}; // SpriteFrame
	
	struct SpriteClip
	{
		std::string name;
		unsigned short frame_start;
		unsigned short total_frames;
		SpriteFrame * frames;
		
		SpriteClip();
		~SpriteClip();
		
		void create_frames( unsigned int material_id, unsigned int num_frames, unsigned int sprite_width, unsigned int sprite_height );
		void purge_frames();
		float * uvs_for_frame( unsigned short frame_id );
		bool is_valid_frame(unsigned short frame_id);
	}; // SpriteClip
	
	
	const int SPRITE_MAX_ATTACHMENTS = 2;
	
	struct SpriteConfig : public Asset
	{
		unsigned int material_id;
		unsigned short width;
		unsigned short height;
		
		
		unsigned short collision_size;
		float frame_delay;
		glm::vec2 scale;
		glm::vec3 attachments[2];
			
		SpriteClip * animations;
		unsigned short total_animations;
	
		
		SpriteClip * get_clip_by_index( unsigned short index );
		void create_animations( unsigned short num_animations );
		void purge_animations();
		virtual void release();
	}; // SpriteConfig
		

	AssetLoadStatus spriteconfig_load_callback( const char * path, SpriteConfig * sprite_config, const AssetParameters & parameters );
	void spriteconfig_construct_extension( StackString<MAX_PATH_SIZE> & extension );

	DECLARE_ASSET_LIBRARY_ACCESSOR(SpriteConfig, AssetParameters, sprites);
}; // namespace assets
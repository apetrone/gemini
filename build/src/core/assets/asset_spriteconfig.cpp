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
#include "stackstring.hpp"
#include "assets.hpp"
#include "assets/asset_spriteconfig.hpp"
#include "configloader.hpp"
#include "render_utilities.hpp"

namespace assets
{
	SpriteClip::SpriteClip()
	{
		this->frame_start = 0;
		this->frames = 0;
		this->total_frames = 0;
	}
	
	SpriteClip::~SpriteClip()
	{
		purge_frames();
	}
	
	void frame_to_pixels( unsigned short frame, assets::Texture * texture, unsigned int sprite_width, unsigned int sprite_height, unsigned int & x, unsigned int & y )
	{
		unsigned short cols = (texture->width / sprite_width);
		unsigned short rows = (texture->height / sprite_height);
		
		x = (frame % cols) * sprite_width;
		y = (frame / rows) * sprite_height;
	} // frame_to_pixels
	
	void SpriteClip::create_frames(unsigned int material_id, unsigned int num_frames, unsigned int sprite_width, unsigned int sprite_height)
	{
		// ...
		
		assets::Material * material = assets::materials()->find_with_id(material_id);
		if ( !material )
		{
			LOGE( "Unable to locate material with id: %i. Cannot load frames!\n", material_id );
			return;
		}
		
		assets::Material::Parameter * parameter = material->parameter_by_name("diffusemap");
		if ( !parameter )
		{
			LOGE( "Unable to find parameter by name: diffusemap\n" );
			return;
		}
		
		assets::Texture * texture = assets::textures()->find_with_id(parameter->intValue);
		if ( !texture )
		{
			LOGE( "Unable to find texture for id: %i\n", parameter->intValue );
			return;
		}
		
		if ( this->frames && total_frames > 0 )
		{
			purge_frames();
		}
		
		total_frames = num_frames;
		this->frames = CREATE_ARRAY(SpriteFrame, total_frames);
		
		
		unsigned int x = 0;
		unsigned int y = 0;
		for( unsigned int frame = 0; frame < total_frames; ++frame )
		{
			SpriteFrame * sf = &frames[ frame ];
			frame_to_pixels( frame+frame_start, texture, sprite_width, sprite_height, x, y );
			render_utilities::sprite::calc_tile_uvs( (float*)sf->texcoords, x, y, sprite_width, sprite_height, texture->width, texture->height );
		}
		
	} // load_frames
	
	void SpriteClip::purge_frames()
	{
		DESTROY_ARRAY(SpriteFrame, frames, total_frames);
		total_frames = 0;
	} // purge_frames
	
	float * SpriteClip::uvs_for_frame(unsigned short frame_id)
	{
		if (is_valid_frame(frame_id))
		{
			return (float*)&frames[ frame_id ].texcoords;
		}
		
		return 0;
	} // uvs_for_frame
	
	bool SpriteClip::is_valid_frame(unsigned short frame_id)
	{
		return (frame_id >= 0 && frame_id < total_frames);
	} // is_valid_frame
	
	
	

	SpriteClip * SpriteConfig::get_clip_by_index( unsigned short index )
	{
		if ( index >= 0 && index < this->total_animations )
			return &animations[ index ];
		
		return 0;
	} // get_clip_by_index
	
	
	void SpriteConfig::create_animations( unsigned short num_animations )
	{
		if ( animations && total_animations > 0 )
		{
			purge_animations();
		}
		
		animations = CREATE_ARRAY(SpriteClip, num_animations);
		total_animations = num_animations;
	} // create_animations
	
	void SpriteConfig::purge_animations()
	{
		DESTROY_ARRAY(SpriteClip, animations, total_animations);
	} // purge_animations

	void SpriteConfig::release()
	{
		purge_animations();
	} // release
	
	
	util::ConfigLoadStatus load_sprite_from_file( const Json::Value & root, void * data )
	{

		SpriteConfig * sprite = (SpriteConfig*)data;
		if (!sprite)
		{
			return util::ConfigLoad_Failure;
		}

		// check material path; load material and cache the id in the sprite
		Json::Value material_path = root["material"];
		if ( material_path.isNull() )
		{
			LOGE( "material is required\n" );
			return util::ConfigLoad_Failure;
		}
		assets::Material * material = assets::materials()->load_from_path( material_path.asString().c_str() );
		if ( material )
		{
			sprite->material_id = material->Id();
		}
	
		// check width and height; set these in the sprite
		Json::Value width_pixels = root["width"];
		Json::Value height_pixels = root["height"];
		if ( width_pixels.isNull() || height_pixels.isNull() )
		{
			LOGE( "width and height are required\n" );
			return util::ConfigLoad_Failure;
		}
		sprite->width = width_pixels.asUInt();
		sprite->height = height_pixels.asUInt();
		
		// load collision size
		Json::Value collision_width = root["collision_size"];
		if ( collision_width.isNull() )
		{
			LOGE( "collision_size is required\n" );
			return util::ConfigLoad_Failure;
		}
		sprite->collision_size = collision_width.asUInt();
		
		// frame delay
		Json::Value frame_delay = root["frame_delay"];
		if ( !frame_delay.isNull() )
		{
			sprite->frame_delay = frame_delay.asFloat();
		}
		
		// load sprite scale
		Json::Value scale_x = root["scale_x"];
		Json::Value scale_y = root["scale_y"];
		if ( !scale_x.isNull() )
		{
			sprite->scale.x = scale_x.asFloat();
		}
		
		if ( !scale_y.isNull() )
		{
			sprite->scale.y = scale_y.asFloat();
		}
		
		
		

		// check for and load all animations
		Json::Value animation_list = root["animations"];
		if ( animation_list.isNull() )
		{
			LOGE( "TODO: handle no animations with a static frame...\n" );
			return util::ConfigLoad_Failure;
		}
		
		sprite->create_animations( animation_list.size() );
		unsigned short animation_index = 0;
		
		Json::ValueIterator iter = animation_list.begin();
		for( ; iter != animation_list.end(); ++iter )
		{
			Json::Value animation = (*iter);
			Json::Value animation_name = animation["name"];
			Json::Value frame_start = animation["frame_start"];
			Json::Value num_frames = animation["num_frames"];
			if ( animation_name.isNull() )
			{
				LOGE( "'name' is required for animation!\n" );
				continue;
			}
			
			if ( frame_start.isNull() )
			{
				LOGE( "'frame_start' is required for animation!\n" );
				continue;
			}
			
			if ( num_frames.isNull() )
			{
				LOGE( "'num_frames' is required for animation!\n" );
			}
			
			SpriteClip * clip = sprite->get_clip_by_index( animation_index++ );
			clip->name = animation_name.asString();
			clip->frame_start = frame_start.asInt();
			clip->total_frames = num_frames.asInt();
			
			// this can be deferred when I merge sprite sheets
			clip->create_frames( sprite->material_id, clip->total_frames, sprite->width, sprite->height );
			//		LOGV( "animation name: %s\n", animation_name.asString().c_str() );
			//		LOGV( "start: %i, num_frames: %i\n", start.asInt(), num_frames.asInt() );
		}

		return util::ConfigLoad_Success;
	} // load_sprite_from_file

	AssetLoadStatus spriteconfig_load_callback( const char * path, SpriteConfig * sprite_config, unsigned int flags )
	{
		if ( util::json_load_with_callback(path, load_sprite_from_file, sprite_config, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // texture_load_callback
	
	
	void spriteconfig_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = "conf";
	} // texture_construct_extension

}; // namespace assets

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

namespace assets
{
	void SpriteConfig::release()
	{
		
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
		
		
		
#if 0
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
			
			Sprite::AnimationSequence * sequence = sprite->get_animation_by_index( animation_index++ );
			sequence->name = animation_name.asString();
			sequence->frame_start = frame_start.asInt();
			sequence->total_frames = num_frames.asInt();
			
			// this can be deferred when I merge sprite sheets
			sequence->create_frames( sprite->material_id, sequence->total_frames, sprite->width, sprite->height );
			//		LOGV( "animation name: %s\n", animation_name.asString().c_str() );
			//		LOGV( "start: %i, num_frames: %i\n", start.asInt(), num_frames.asInt() );
		}
		
#endif
	
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

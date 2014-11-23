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
#include <platform/typedefs.h>
#include <core/configloader.h>
#include <core/stackstring.h>

#include "assets.h"
#include "assets/asset_emitter.h"
#include "renderer/render_utilities.h"

namespace assets
{
	EmitterConfig::EmitterConfig()
	{
		max_particles = 0;
		material_id = 0;
		life.set_range(0, 1);
		size.set_range(0, 1);
	}

	void EmitterConfig::release()
	{
	} // release
	
	
	void color_value( Json::Value & value, Color & out )
	{
		std::string temp = value.asString();
		
		uint32_t colors[4] = { 255, 255, 255, 255 };
		
		// try RGBA
		if (sscanf(temp.c_str(), "%i, %i, %i, %i", &colors[0], &colors[1], &colors[2], &colors[3]) < 4)
		{
			// try just RGB
			sscanf(temp.c_str(), "%i, %i, %i", &colors[0], &colors[1], &colors[2] );
		}

		out.r = colors[0];
		out.g = colors[1];
		out.b = colors[2];
		out.a = colors[3];
	}
	
	void float_value( Json::Value & value, float & out )
	{
		out = value.asFloat();
	}
	
	template <class Type>
	void read_channel_frames( Type * data, Json::Value & frames, void (*func_ptr)(Json::Value & value, Type & out) )
	{
		for(unsigned int index = 0; index < frames.size(); ++index)
		{
			func_ptr(frames[index], data[index]);
		}
	}
	
	util::ConfigLoadStatus load_emitter_from_file( const Json::Value & root, void * data )
	{
		EmitterConfig * cfg = (EmitterConfig*)data;
		if (!cfg)
		{
			return util::ConfigLoad_Failure;
		}
		
		Json::Value max_particles = root["max_particles"];
		Json::Value spawn_rate = root["spawn_rate"];
		Json::Value spawn_delay_seconds = root["spawn_delay_seconds"];
		Json::Value life_min = root["life_min"];
		Json::Value life_max = root["life_max"];
		Json::Value velocity_min = root["velocity_min"];
		Json::Value velocity_max = root["velocity_max"];
		glm::vec3 vmin, vmax;
				
		Json::Value material = root["material"];
		
		if (max_particles.isNull() || spawn_rate.isNull() ||
			spawn_delay_seconds.isNull() || life_min.isNull() ||
			life_max.isNull() || velocity_min.isNull() ||
			velocity_max.isNull() || material.isNull() )
		{
			LOGE("Missing one or more required parameters from emitter configuration. Aborting.\n");
			return util::ConfigLoad_Failure;
		}
		
		
		cfg->max_particles = max_particles.asInt();
		cfg->spawn_rate = spawn_rate.asInt();
		cfg->spawn_delay_seconds = spawn_delay_seconds.asFloat();
		cfg->life.set_range(life_min.asFloat(), life_max.asFloat());
		
		// parse and set velocity
		{
			std::string temp = velocity_min.asString();
			sscanf(temp.c_str(), "%f,%f,%f", &vmin.x, &vmin.y, &vmin.z);
			
			temp = velocity_max.asString();
			sscanf(temp.c_str(), "%f,%f,%f", &vmax.x, &vmax.y, &vmax.z);
			cfg->velocity.set_range(vmin, vmax);
		}
		
		assets::Material* emitter_material = assets::materials()->load_from_path(material.asString().c_str());
		if ( emitter_material )
		{
			cfg->material_id = emitter_material->Id();
		}
		
		// load channel data
		Json::Value channels = root["channels"];
		
		Json::ValueIterator citer = channels.begin();
		for( ; citer != channels.end(); ++citer )
		{
			Json::Value object = (*citer);
//			LOGV("object: %s\n", citer.key().asString().c_str());
			Json::Value frame_delay_seconds = object["frame_delay_seconds"];
			Json::Value frames = object["frames"];
//			LOGV("total items: %i\n", frames.size());
			if (citer.key().asString() == "color")
			{
				Color * colors = CREATE_ARRAY(Color, frames.size());
				read_channel_frames(colors, frames, color_value);
				cfg->color_channel.create(frames.size(), colors, frame_delay_seconds.asFloat());
				DESTROY_ARRAY(Color, colors, frames.size());
			}
			else if(citer.key().asString() == "alpha")
			{
				float * data = (float*)ALLOC(sizeof(float)*frames.size());
				read_channel_frames(data, frames, float_value);
				cfg->alpha_channel.create(frames.size(), data, frame_delay_seconds.asFloat());
				DEALLOC(data);
			}
			else if(citer.key().asString() == "size")
			{
				float * data = (float*)ALLOC(sizeof(float)*frames.size());
				read_channel_frames(data, frames, float_value);
				cfg->size_channel.create(frames.size(), data, frame_delay_seconds.asFloat());
				DEALLOC(data);
			}
		}
		

		return util::ConfigLoad_Success;
	} // load_emitter_from_file

	AssetLoadStatus emitterconfig_load_callback( const char * path, EmitterConfig * config, const AssetParameters & parameters )
	{
		if ( util::json_load_with_callback(path, load_emitter_from_file, config, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // emitterconfig_load_callback
	
	
	void emitterconfig_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = ".conf";
	} // emitterconfig_construct_extension

}; // namespace assets

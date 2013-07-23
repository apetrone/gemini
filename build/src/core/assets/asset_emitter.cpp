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
#include "assets/asset_emitter.hpp"
#include "configloader.hpp"
#include "render_utilities.hpp"

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
	
	
	util::ConfigLoadStatus load_emitter_from_file( const Json::Value & root, void * data )
	{
		EmitterConfig * cfg = (EmitterConfig*)data;
		if (!cfg)
		{
			return util::ConfigLoad_Failure;
		}
		
		
		
		

		return util::ConfigLoad_Success;
	} // load_emitter_from_file

	AssetLoadStatus emitterconfig_load_callback( const char * path, EmitterConfig * sprite_config, unsigned int flags )
	{
		if ( util::json_load_with_callback(path, load_emitter_from_file, sprite_config, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // texture_load_callback
	
	
	void emitterconfig_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = "conf";
	} // texture_construct_extension

}; // namespace assets

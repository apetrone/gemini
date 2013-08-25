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

#include "keyframechannel.hpp"
#include "util.hpp"

namespace assets
{
	struct EmitterConfig : public Asset
	{
		unsigned short max_particles;
		unsigned short spawn_rate;
	
		unsigned int material_id;
		float spawn_delay_seconds;

		RangedValue<unsigned int> life;
		RangedValue<glm::vec3> velocity;
		RangedValue<float> size;

		KeyframeChannel<Color> color_channel;
		KeyframeChannel<float> alpha_channel;
		KeyframeChannel<float> size_channel;

		EmitterConfig();
		virtual void release();
	}; // EmitterConfig

	AssetLoadStatus emitterconfig_load_callback( const char * path, EmitterConfig * config, const AssetParameters & parameters );
	void emitterconfig_construct_extension( StackString<MAX_PATH_SIZE> & extension );

	DECLARE_ASSET_LIBRARY_ACCESSOR(EmitterConfig, AssetParameters, emitters);
}; // namespace assets
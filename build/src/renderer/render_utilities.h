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

namespace gemini
{
	namespace assets
	{
		struct Geometry;
	};

	struct RenderStream;

	namespace assets
	{
		struct Shader;
	};

	namespace renderer
	{
		struct RenderBlock;
		struct ConstantBuffer;
	};
} // namespace gemini

#include "renderer.h"

namespace gemini
{
	namespace render_utilities
	{
		//
		// misc sprite tools
		namespace sprite
		{
			void calc_tile_uvs( float * uvs, unsigned int x, unsigned int y, unsigned int sprite_width, unsigned int sprite_height, unsigned int sheet_width, unsigned int sheet_height );
		}; // sprite
		
		
		template <class Type>
		struct PhysicsState
		{
			Type last;
			Type current;
			Type render;
			
			void snap( const Type & value )
			{
				render = current = last = value;
			}
			
			void step( float delta_sec )
			{
				last = current;
			}
			
			void interpolate( float t )
			{
				Interpolator<Type> interpolator;
				render = interpolator( last, current, t );
			}
		}; // PhysicsState
		
		
		// strip the version line from shader source
		void strip_shader_version( char * buffer, StackString<32> & version );
	} // namespace render_utilities
} // namespace gemini






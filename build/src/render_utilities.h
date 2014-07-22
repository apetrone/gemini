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

#include "mathlib.h"

template <class Type>
Type lerp( const Type & a, const Type & b, float t )
{
	return glm::mix( a, b, t );
}


// -------------------------------------------------------------
template <class Type>
struct Interpolator
{
	Type operator()( const Type & start, const Type & end, float t )
	{
		// return the linearly interpolated value
		return lerp( start, end, t );
	}
};



template <class Type>
Type slerp( const Type & a, const Type & b, float t )
{
	return glm::mix( a, b, t );
}


// -------------------------------------------------------------
template <>
struct Interpolator<glm::quat>
{
	glm::quat operator()( const glm::quat& start, const glm::quat& end, float t )
	{
		// return the linearly interpolated value
		return slerp(start, end, t);
	}
};



namespace renderer
{
	struct GeneralParameters;
};

namespace assets
{
	struct Geometry;
};

struct RenderStream;

namespace assets
{
	struct Shader;
};

namespace render_utilities
{
	void stream_geometry( RenderStream & rs, assets::Geometry * geo, renderer::GeneralParameters & gp, assets::Shader* shader = 0 );
	
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
}; // namespace render_utilities








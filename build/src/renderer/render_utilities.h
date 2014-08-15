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

#include <gemini/mathlib.h>

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




inline glm::quat custom_slerp(const glm::quat& q1, const glm::quat& q2, float t)
{
	glm::quat out;
	glm::quat q2b;
	
	float sq1, sq2;
	float cosom = q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];
	
	if( cosom < 0.0f)
	{
		cosom = -cosom;
		q2b[0] = -q2[0];
		q2b[1] = -q2[1];
		q2b[2] = -q2[2];
		q2b[3] = -q2[3];
	}
	else
	{
		//QuaternionCopy(q2, q2b);
		q2b = q2;
	}
	
	if( (1.0f + cosom) > 1E-5)
	{
		if( (1.0f - cosom) > 1E-5)
		{
			float om = (float) acos(cosom);
			float rsinom = (float)(1.0f / sin(om));
			
			sq1 = (float)sin( (1.0f - t) * om) * rsinom;
			sq2 = (float)sin(t * om) * rsinom;
		}
		else
		{
			sq1 = (float)(1.0f - t);
			sq2 = t;
		}
		
		out[3] = sq1 * q1[3] + sq2 * q2b[3];
		out[0] = sq1 * q1[0] + sq2 * q2b[0];
		out[1] = sq1 * q1[1] + sq2 * q2b[1];
		out[2] = sq1 * q1[2] + sq2 * q2b[2];
	}
	else
	{
		const float PI = (float)3.14159265358979323846f;
		
		sq1 = (float)sin( (1.0f - t) * 0.5f * PI);
		sq2 = (float)sin(t * 0.5f * PI);
		
		out[3] = sq1 * q1[3] + sq2 * q1[2];
		out[0] = sq1 * q1[0] + sq2 * q1[1];
		out[1] = sq1 * q1[1] + sq2 * q1[0];
		out[2] = sq1 * q1[2] + sq2 * q1[3];
	}
	
	return out;
}


template <class Type>
Type slerp( const Type & a, const Type & b, float t )
{
	return custom_slerp(a, b, t);
	
	// glm::mix has a bug where if the angles of the quaternions are too close;
	// they 'mix' to an invalid quaternion (NaN, NaN, NaN, NaN)
	//return glm::mix( a, b, t );
	
//	Groovounet: If you need a slerp that always take the short path, let me recommend to you to use shortMix.
//	return glm::shortMix(a, b, t);
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

namespace render_utilities
{
	void queue_geometry(RenderStream& rs, const renderer::RenderBlock& block, const renderer::ConstantBuffer& constant_buffer);

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








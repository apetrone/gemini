// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone

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

#include <color.hpp>

// -------------------------------------------------------------
inline float lerp( float a, float b, float t )
{
	return (a + (b-a)*t);
}

// -------------------------------------------------------------

template <class Type>
struct KeyframeInterpolator
{
	Type operator()( const Type & start, const Type & end, float t )
	{
		// return the linearly interpolated value
		return lerp( start, end, t );
	}
};

template <>
struct KeyframeInterpolator<Color>
{
	Color operator()( const Color & start, const Color & end, float t )
	{
		return Color( lerp( start.r, end.r, t ),
							  lerp( start.g, end.g, t ),
							  lerp( start.b, end.b, t ),
							  lerp( start.a, end.a, t ) );
	}
};

// -------------------------------------------------------------



template <class Type, class Interpolator=KeyframeInterpolator<Type> >
struct KeyframeChannel
{
	unsigned int total_samples;
	float frame_delay;
	Type pre_infinity;
	Type post_infinity;
	Type * samples;
	static Interpolator interpolator;
	
	KeyframeChannel()
	{
		samples = 0;
		total_samples = 0;
	}
	
	~KeyframeChannel()
	{
		if ( samples )
		{
			delete [] samples;
			samples = 0;
		}
		
		total_samples = 0;
	}
	
	void create( unsigned int max_samples, const Type * values, float framedelay, Type pre_infinity_value = Type(), Type post_infinity_value = Type() )
	{
		samples = new Type[ total_samples ];
		total_samples = total_samples;
		
		set_keys( values, total_samples );
		
		pre_infinity = samples[0];
		post_infinity = samples[ total_samples-1 ];
		
		//_frametime = 1.0 / (float)(total_samples-1);
		frame_delay = framedelay;
	}
	
	void set_keys( const Type * ptr, unsigned int num_samples, unsigned int offset = 0 )
	{
		unsigned int samples_to_copy = num_samples;
		if ( samples_to_copy > total_samples )
			samples_to_copy = total_samples;
		
		memcpy( &samples[offset], ptr, samples_to_copy*sizeof(Type) );
	}
	
	void set_keyframe( unsigned int index, const Type & value )
	{
		if ( index < total_samples )
		{
			samples[ index ] = value;
		}
	}
	
	const Type get_value( float t )
	{
		if (t < 0)
		{
			return pre_infinity;
		}
		else if (t > 1.0)
		{
			return post_infinity;
		}
		
		// find the index of the sample, roughly matching with t
		//		int index = (t * ((float)_total_samples-1));
		int index = (t / frame_delay);
		
		// calculate the start time of the frame at index
		float start_time = (index * frame_delay);
		
		// calculate [0,1] how far between index and index+1 keyframes
		float n = (t - start_time) / frame_delay;
		
		if (index >= (total_samples-1))
		{
			return post_infinity;
		}
		
		// now just interpolate using n
		return interpolator(samples[index], samples[index+1], n);
	}
}; // KeyframeChannel

template <class Type, class Interpolator >
Interpolator KeyframeChannel<Type, Interpolator>::interpolator;
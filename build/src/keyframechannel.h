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

#include "render_utilities.h"
#include "color.h"
#include <gemini/util/fixedarray.h>
#include "render_utilities.h"


template <class Type>
struct KeyframeData
{
	FixedArray<Type> keys;
		
	void set_keys(Type* data, size_t total_keys)
	{
		keys.allocate(total_keys);
		memcpy(&keys[0], data, sizeof(Type)*total_keys);
	}
};
// -------------------------------------------------------------




template <class Type, class Interpolator=Interpolator<Type> >
class Channel
{
	Type& value;
//	FixedArray<Type> keys;
	KeyframeData<Type>* data_source;
	static Interpolator interpolator;
	
public:
	Channel(Type& value_in, KeyframeData<Type>* source = 0) :
		value(value_in), data_source(source)
	{
		
	}
	~Channel() {}

	void set_data_source(KeyframeData<Type>* datasource)
	{
		data_source = datasource;
		assert(data_source != 0);
	}
	
	Type get_value(uint32_t frame, float alpha)
	{
		alpha = glm::clamp(alpha, 0.0f, 1.0f);
		
		frame = clamp_frame(frame);
		
		assert(data_source && data_source->keys.size() > 0);
		
		float last = data_source->keys[frame];
		float next;
		if ((frame+1) >= data_source->keys.size())
		{
			// TODO: Should use post-infinity here
			// For now, just use the max.
			next = data_source->keys[frame];
		}
		else
		{
			next = data_source->keys[frame+1];
		}
		
		float delta = (next-last);
		
		// interpolate between frame and frame+1
		value = Channel<Type, Interpolator>::interpolator(last, delta, alpha);
		return value;
	}
	
private:
	uint32_t clamp_frame(uint32_t frame)
	{
		assert(data_source != 0);
		if (frame >= data_source->keys.size())
		{
			frame = data_source->keys.size()-1;
		}
		return frame;
	}
};

template <class Type, class Interpolator>
Interpolator Channel<Type, Interpolator>::interpolator;


template <class Type, class Interpolator=Interpolator<Type> >
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
			DESTROY_ARRAY(Type, samples, total_samples);
			samples = 0;
		}
		
		total_samples = 0;
	}
	
	void create( unsigned int max_samples, const Type * values, float framedelay, Type pre_infinity_value = Type(), Type post_infinity_value = Type() )
	{
		total_samples = max_samples;
		samples = CREATE_ARRAY(Type, total_samples);
		
		
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
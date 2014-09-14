// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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
#include "keyframechannel.h"



#if 0

Channel::Channel(float& value_in) :
	value(value_in)
{
	
}

void Channel::set_keys(float* data, size_t total_keys)
{
	keys.allocate(total_keys);
	memcpy(&keys[0], data, sizeof(float)*total_keys);
}

float Channel::get_value(uint32_t frame, float alpha)
{
	alpha = glm::clamp(alpha, 0.0f, 1.0f);
	
	frame = clamp_frame(frame);
	
	assert(keys.size() > 0);
	
	float last = keys[frame];
	float next;
	if ((frame+1) >= keys.size())
	{
		// TODO: Should use post-infinity here
		// For now, just use the max.
		next = keys[frame];
	}
	else
	{
		next = keys[frame+1];
	}
	
	float delta = (next-last);
	
	// interpolate between frame and frame+1
	return glm::mix(last, delta, alpha);
}


uint32_t Channel::clamp_frame(uint32_t frame)
{
	if (frame >= keys.size())
	{
		frame = keys.size()-1;
	}
	return frame;
}

#endif


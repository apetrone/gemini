// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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


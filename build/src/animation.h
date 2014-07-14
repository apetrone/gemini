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

namespace animation
{
	typedef unsigned int BoneIndex;
	struct VertexWeight
	{
		BoneIndex bone_index;
		float weight;
		
		VertexWeight(BoneIndex boneid, float vertex_weight) :
			bone_index(boneid), weight(vertex_weight)
		{
			
		}
	}; // VertexWeight
	
	struct BlendInfo
	{
		unsigned int weight_index;
		unsigned int weight_count;
		
		BlendInfo(unsigned int weightid, unsigned int num_weights) :
			weight_index(weightid), weight_count(num_weights)
		{
			
		}
	}; // BlendInfo
	
	struct AnimationNode // : public RenderNode ?
	{
		virtual void update(float delta_seconds) {}
	};
}; // namespace animation



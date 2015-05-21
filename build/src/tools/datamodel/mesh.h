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
#pragma once

#include "datamodel/material.h"

#include <fixedarray.h>
#include <core/mathlib.h>

namespace gemini
{
	namespace datamodel
	{
		const int MAX_SUPPORTED_UV_CHANNELS = 2;
		const int MAX_SUPPORTED_BONE_INFLUENCES = 4;
		
		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec4 color;
			glm::vec2 uv[MAX_SUPPORTED_UV_CHANNELS];
		};
		
		struct Weight
		{
			String bone_name;
			float value;
			
			Weight() : value(0.0f)
			{
			}
		};
		
		struct WeightList
		{
			Weight weights[MAX_SUPPORTED_BONE_INFLUENCES];
			uint8_t total_weights;
			
			WeightList() : total_weights(0)
			{
			}
		};
		
		
		struct BoneLinkData
		{
			String bone_name;
			glm::mat4 inverse_bind_pose;
			int32_t parent;
		};
		
		struct Mesh
		{
			FixedArray<glm::vec4> blend_indices;
			FixedArray<glm::vec4> blend_weights;
			FixedArray<glm::vec3> vertices;
			FixedArray<glm::vec3> normals;
			FixedArray<glm::vec4> vertex_colors;
			FixedArray< FixedArray<glm::vec2> > uvs;
			FixedArray<WeightList> weights;
			FixedArray<BoneLinkData> bindpose;
			FixedArray<uint32_t> indices;
			MaterialId material;
			glm::vec3 mass_center_offset;
			std::string name;

			Mesh()
			{
				material = 0;
			}

			~Mesh()
			{
			}
		};
	} // namespace datamodel
} // namespace gemini
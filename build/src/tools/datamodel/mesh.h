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

#include <core/fixedarray.h>
#include <core/mathlib.h>

#include "datamodel/material.h"

namespace gemini
{
	namespace datamodel
	{
		const int MAX_SUPPORTED_UV_CHANNELS = 2;
		
		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec4 color;
			glm::vec2 uv[MAX_SUPPORTED_UV_CHANNELS];
		};
		
		
		struct Mesh
		{
			core::FixedArray<glm::vec4> blend_indices;
			core::FixedArray<glm::vec4> blend_weights;
			core::FixedArray<glm::vec3> vertices;
			core::FixedArray<glm::vec3> normals;
			core::FixedArray<glm::vec4> vertex_colors;
			core::FixedArray< core::FixedArray<glm::vec2> > uvs;
			core::FixedArray<uint32_t> indices;
			MaterialId material;
			glm::vec3 mass_center_offset;

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
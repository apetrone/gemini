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
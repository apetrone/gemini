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

#include <core/mathlib.h>
#include <platform/mem_stl_allocator.h>
#include <vector>
#include <string>

#include "texture.h"

namespace gemini
{
	namespace renderer
	{
		// must also make a change in: MaterialParameterTypeToRenderState
		enum MaterialParameterType
		{
			MP_INT = 0,
			MP_SAMPLER_2D,
			MP_SAMPLER_CUBE,
			MP_VEC4
		};
		
		
		struct MaterialParameter
		{
			std::string name;
			std::string value;
			unsigned int type; // MaterialParameterType
			int int_value;
			glm::vec4 vector_value;
			unsigned int texture_unit;
			renderer::Texture* texture;
		}; // MaterialParameter

		struct Material
		{
			enum
			{
				BLENDING = 1,
				SHADOWMAP = 2,
				CUBEMAP = 4,
			};
			
			std::string name;
			std::vector< MaterialParameter, GeminiAllocator<MaterialParameter> > parameters;
			unsigned int flags;
			unsigned int requirements; // used to lookup the correct shader permutation for this material
			
			
			// this will generate a value based on the parameters applied
			// to this material such that the correct shader can be found and used when rendering
			void calculate_requirements();
			
			MaterialParameter * parameter_by_name(const std::string& name);
			
			void set_parameter_name( unsigned int id, const char * name );
			void set_parameter_vec4( unsigned int id, const glm::vec4 & vec );
			
			void add_parameter(const renderer::MaterialParameter& param);
			
			void print_parameters();
		};
	} // namespace renderer
} // namespace gemini
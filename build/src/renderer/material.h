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

#include "texture.h"

#include <core/mathlib.h>
#include <platform/mem_stl_allocator.h>

#include <vector>
#include <string>

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
			std::vector< MaterialParameter, CustomPlatformAllocator<MaterialParameter> > parameters;
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
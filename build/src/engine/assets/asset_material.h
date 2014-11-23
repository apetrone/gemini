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

#include <vector>

#include <core/stackstring.h>

#include "assets.h"
#include "assets/asset_shader.h"

namespace assets
{
	

	
	// must also make a change in: MaterialParameterTypeToRenderState
	enum MaterialParameterType
	{
		MP_INT = 0,
		MP_SAMPLER_2D,
		MP_SAMPLER_CUBE,
		MP_VEC4
	};
	
	struct Material : public Asset
	{
		struct Parameter
		{
			renderer::ShaderString name;
			renderer::ShaderString value;
			unsigned int type; // MaterialParameterType
			int intValue;
			glm::vec4 vecValue;
			unsigned int texture_unit;
		}; // Parameter
		
		enum
		{
			BLENDING = 1,
			SHADOWMAP = 2,
			CUBEMAP = 4,
		};
		
		renderer::ShaderString name;
		Shader * shader;
		std::vector< Parameter, GeminiAllocator<Parameter> > parameters;

		unsigned int flags;
		unsigned int requirements; // used to lookup the correct shader permutation for this material
		virtual void release();
		
		// this will generate a value based on the parameters applied
		// to this material such that the correct shader can be found and used when rendering
		void calculate_requirements();
		
		Parameter * parameter_by_name( const char * name );

		void set_parameter_name( unsigned int id, const char * name );
		void set_parameter_vec4( unsigned int id, const glm::vec4 & vec );
		
		void add_parameter(const Material::Parameter& param);
		
		void print_parameters();
	}; // Material
	
	unsigned int texture_unit_for_map( renderer::ShaderString & name );
	
	unsigned int material_type_to_parameter_type( const char * name );
	int material_parameter_type_to_render_state( unsigned int type );
		
	
	AssetLoadStatus material_load_callback( const char * path, Material * material, const AssetParameters & parameters );
	void material_construct_extension( StackString<MAX_PATH_SIZE> & extension );

	DECLARE_ASSET_LIBRARY_ACCESSOR(Material, AssetParameters, materials);
}; // namespace assets
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

#include "assets.hpp"
#include "stackstring.hpp"
#include "assets/asset_shader.hpp"

namespace assets
{
	
	// -------------------------------------------------------------
	// Material
	
	// must also make a change in: MaterialParameterTypeToRenderState
	enum MaterialParameterType
	{
		MP_INT = 0,
		MP_SAMPLER_2D,
		MP_SAMPLER_CUBE,
		MP_VEC4
	};
	
	struct Material : public virtual Asset
	{
		struct Parameter
		{
			ShaderString name;
			ShaderString value;
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
		
		ShaderString name;
		Shader * shader;
		Parameter * parameters;
		//		unsigned int texture_id;
		unsigned int flags;
		unsigned int num_parameters;
		unsigned int requirements; // used to lookup the correct shader permutation for this material
		virtual void release();
		
		// this will generate a value based on the parameters applied
		// to this material such that the correct shader can be found and used when rendering
		void calculate_requirements();
		
		Parameter * parameter_by_name( const char * name );
		
		void allocate_parameters( unsigned int max_parameters );
		void set_parameter_name( unsigned int id, const char * name );
		void set_parameter_vec4( unsigned int id, const glm::vec4 & vec );
	}; // Material
	
	Material * material_by_id( unsigned int id );
	Material * load_material( const char * path, unsigned int flags = 0, bool ignore_cache = false );

	
	unsigned int find_parameter_mask( ShaderString & name );
	unsigned int texture_unit_for_map( ShaderString & name );
	
	unsigned int material_type_to_parameter_type( const char * name );
	int material_parameter_type_to_render_state( unsigned int type );
	
	
	
	AssetLoadStatus material_load_callback( const char * path, Material * material, unsigned int flags );
	void material_construct_extension( StackString<MAX_PATH_SIZE> & extension );
	
	
	typedef AssetLibrary< Material, MaterialAsset> MaterialAssetLibrary;
	MaterialAssetLibrary * materials();
}; // namespace assets
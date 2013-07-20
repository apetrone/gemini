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
#include "renderer.hpp"

namespace assets
{
	// -------------------------------------------------------------
	// Shader
	
	
	struct KeyHashPair
	{
		ShaderString key;
		int value;
	};
	
	struct Shader : public virtual Asset, public virtual renderer::ShaderParameters, public virtual renderer::ShaderProgram
	{
		int get_uniform_location( const char * name );
		virtual void release();
		
		void create_program();
		void destroy_program();
		void attach_shader( /*GLObject shader*/ );
		void bind_attributes();
		bool link_and_validate();
		void bind_uniforms();
	}; // Shader
	
	
	struct ShaderPermutationGroup
	{
		ShaderString name;
		
		unsigned int num_defines;
		unsigned int num_attributes;
		unsigned int num_uniforms;
		unsigned int num_requires;
		unsigned int num_conflicts;
		
		ShaderString * defines;
		ShaderString * attributes;
		ShaderString * uniforms;
		ShaderString * requires;
		ShaderString * conflicts;
		
		unsigned int mask_value;
		
		ShaderPermutationGroup();
		~ShaderPermutationGroup();
	}; // ShaderPermutationGroup
	
	
	struct ShaderPermutations
	{
		ShaderPermutationGroup base;
		
		unsigned int num_permutations;
		ShaderPermutationGroup ** options;
		
		unsigned int num_attributes;
		ShaderPermutationGroup * attributes;
		
		unsigned int num_uniforms;
		ShaderPermutationGroup * uniforms;
		
		StackString<64> frag_location;
		
		// total number of parameters across permutations
		// each one has a unique ID which is the mask value for that parameter
		unsigned short num_parameters;
		
		
		ShaderPermutations();
		~ShaderPermutations();
	}; // ShaderPermutations
	
	ShaderPermutations & shader_permutations();
	void compile_shader_permutations();
	Shader * find_compatible_shader( unsigned int attributes );
	
	renderer::ShaderObject create_shader_from_file( const char * shader_path, renderer::ShaderObjectType type, const char * preprocessor_defines );
	void load_shader( const char * shader_path, Shader * shader );
	void destroy_shader( Shader * shader );
	void load_test_shader( Shader * shader );
}; // namespace assets
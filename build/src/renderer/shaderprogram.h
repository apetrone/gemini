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

#include <core/stackstring.h>
#include <fixedarray.h>

#include <string>
#include <utility> // for std::pair

namespace renderer
{
	enum ShaderObjectType
	{
		SHADER_VERTEX,
		SHADER_FRAGMENT,
		SHADER_GEOMETRY,
		SHADER_COMPUTE,
		
		SHADER_LIMIT
	}; // ShaderObjectType
	
	typedef std::pair<std::string, int> ShaderKeyValuePair;
	
	// Generic container for shader objects before they are linked into a shader
	// program.
	struct ShaderObject
	{
		unsigned int shader_id;
		
		ShaderObject() : shader_id(0) {}
	}; // ShaderObject
	
	struct ShaderProgram
	{
		core::StackString<64> frag_data_location;
		
		FixedArray<ShaderKeyValuePair> uniforms;
		FixedArray<ShaderKeyValuePair> attributes;
		
		unsigned int object;
		
		ShaderProgram() : object(0), frag_data_location("out_color") {}
		virtual ~ShaderProgram() {}
		
		int get_uniform_location( const char * name );
		
		void show_uniforms();
		void show_attributes();
	}; // ShaderProgram
	
	
	
	enum ShaderErrorType
	{
		SHADER_ERROR_NONE = 0,
		SHADER_ERROR_COMPILE_FAILED,
	}; // ShaderErrorType

} // namespace renderer
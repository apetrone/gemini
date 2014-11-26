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

#include <core/stackstring.h>
#include <core/fixedarray.h>

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
		StackString<64> frag_data_location;
		
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

}; // namespace renderer
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


#include "typedefs.h"
#include "renderer.hpp"
#include <string>

namespace assets
{
#if 0
	struct KeyValuePair
	{
		std::string key;
		unsigned int key_hash;
		int value;
	}; // UKeyValuePair
#endif

	struct KeyHashPair
	{
		StackString<32> key;
		int value;
	};
	
	struct Shader : public virtual Asset, public virtual renderer::ShaderParameters
	{	
		unsigned int capabilities;
			
		Shader();
		~Shader();
		
		int get_uniform_location( const char * name );
		virtual void release();
		
		void create_program();
		void destroy_program();
		void attach_shader( /*GLObject shader*/ );
		void bind_attributes();
		bool link_and_validate();
		void bind_uniforms();
	};
	
	
	
	
}; // namespace assets
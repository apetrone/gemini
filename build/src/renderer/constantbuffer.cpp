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
#include "constantbuffer.h"

namespace renderer
{
	ConstantBuffer::ConstantBuffer(ShaderProgram* shader_program) : program(shader_program)
	{
		modelview_matrix = nullptr;
		projection_matrix = nullptr;
		viewer_direction = nullptr;
		viewer_position = nullptr;
		light_position = nullptr;
	}
	
	void ConstantBuffer::add_uniform_matrix4(const char* name, const glm::mat4* data)
	{
		
	}
//	void ConstantBuffer::add_uniform1i(int uniform_location, int value)
//	{
//		
//	}
//	
//	void ConstantBuffer::add_uniform3f(int location, const glm::vec3* data)
//	{
//		
//	}
//	
//	void ConstantBuffer::add_uniform4f(int location, const glm::vec4* data)
//	{
//		
//	}
//	
//	void ConstantBuffer::add_uniform_matrix4(int location, const glm::mat4* data, uint8_t count)
//	{
//		
//	}
}; // namespace renderer
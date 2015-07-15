// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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

#include <renderer/gl/gemgl.h>

#include <core/typedefs.h>

namespace render2
{
	static size_t type_to_bytes(const GLenum& type)
	{
		switch(type)
		{
			case GL_FLOAT_VEC2: return sizeof(GLfloat) * 2;
			case GL_FLOAT_VEC3: return sizeof(GLfloat) * 3;
			case GL_FLOAT_VEC4: return sizeof(GLfloat) * 4;
				
			case GL_FLOAT_MAT4: return sizeof(GLfloat) * 16;
				
				
			default: break;
		}
		
		// If you reach this, you're missing an OpenGL type from the above switch
		assert(0);
		return 0;
	}
	
	const size_t MAX_ATTRIBUTE_NAME_LENGTH = 32;
	struct shader_variable
	{
		// location of this variable
		GLint location;
		
		// byte-length of name
		GLsizei length;
		
		// byte-length of the attribute value
		GLint size;
		
		// attribute name (null-terminated string)
		GLchar name[ MAX_ATTRIBUTE_NAME_LENGTH ];
		
		// data type of the attribute
		GLenum type;
		
		// size (in bytes) of this type
		GLint byte_size;
		
		void compute_size()
		{
			// compute byte size for this attribute
			byte_size = (size * type_to_bytes(type));
		}
	};

	struct VertexDataTypeToGL
	{
		GLenum type;
		GLenum normalized;
		uint32_t element_size;
		
		VertexDataTypeToGL(GLenum _type = GL_INVALID_ENUM, GLenum _normalized = GL_INVALID_ENUM, uint32_t _element_size = 0) :
			type(_type),
			normalized(_normalized),
			element_size(_element_size)
		{
		}
	};
	
	#define VERTEXDATA_TO_GL(vdt, gl_type, gl_normalized, element_size) \
		_vertex_data_to_gl[ vdt ] = VertexDataTypeToGL(gl_type, gl_normalized, element_size)
		
	extern VertexDataTypeToGL _vertex_data_to_gl[ VD_TOTAL ];
} // namespace render2

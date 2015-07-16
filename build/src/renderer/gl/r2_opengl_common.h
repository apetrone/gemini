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

#include "gemgl.h"

#include <core/fixedarray.h>
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

	class GLInputLayout : public InputLayout
	{
	public:
		struct Description
		{
			GLint location;
			GLenum type;
			GLsizei element_count;
			GLenum normalized;
			size_t offset;
			size_t size;
		};
		
		size_t vertex_stride;
		FixedArray<Description> items;
	}; // GLInputLayout
		
	struct GLShader : public Shader
	{
		GLShader();
		virtual ~GLShader();
				
		bool compile_shader(GLint shader, const char* source, const char* preprocessor_defines, const char* version);
		char* query_program_info_log(renderer::GLObject handle);
		void dump_program_log();
		
		int build_from_source(const char *vertex_shader, const char *fragment_shader, const char* preprocessor, const char* version);
		GLint get_attribute_location(const char* name);
		GLint get_uniform_location(const char* name);
		
		GLint id;
		FixedArray<shader_variable> uniforms;
		FixedArray<shader_variable> attributes;
	}; // GLShader

	struct GLPipeline : public Pipeline
	{
		GLShader* program;
		VertexDescriptor vertex_description;
		GLInputLayout* input_layout;
		
		GLPipeline(const PipelineDescriptor& descriptor);
		virtual ~GLPipeline();
	}; // GLPipeline


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

	void setup_input_layout(GLInputLayout* layout, const VertexDescriptor& descriptor, GLShader* shader);
	void setup_pipeline(GLPipeline* pipeline, const PipelineDescriptor& descriptor);
	void populate_vertexdata_table();
	VertexDataTypeToGL* get_vertexdata_table();
} // namespace render2
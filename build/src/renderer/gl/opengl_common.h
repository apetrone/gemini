// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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

#include <stack>
#include <string>

#include <core/stackstring.h>

#include "renderer.h"

#define FAIL_IF_GLERROR( error ) if ( error != GL_NO_ERROR ) { return false; }

#include "gemgl.h" // for GLObject

namespace gemini
{
	typedef GLenum (*get_internal_image_format)(unsigned int);

	GLenum vertexbuffer_drawtype_to_gl_drawtype( renderer::VertexBufferDrawType type );
	GLenum vertexbuffer_buffertype_to_gl_buffertype( renderer::VertexBufferBufferType type );
	GLenum shaderobject_type_to_gl_shaderobjecttype( renderer::ShaderObjectType type );

	// the callee is responsible for deallocating the memory returned from this function
	char * query_shader_info_log( GLObject handle );

	// the callee is responsible for deallocating the memory returned from this function
	char * query_program_info_log( GLObject handle );

	GLenum driver_state_to_gl_state( renderer::DriverState state );
	GLenum convert_blendstate( renderer::RenderBlendType state );

	GLenum cullmode_to_gl_cullmode( renderer::CullMode mode );

	typedef void (*gemgl_state_function)(renderer::DriverState, core::util::MemoryStream&, renderer::IRenderDriver*);

	gemgl_state_function operator_for_state( renderer::DriverState state );

	//#define GL_LOG(...) GLFunctionLogger _gl_log(__FUNCTION__)
	#define GL_LOG(...)
	struct GLFunctionLogger
	{
		const char* function;
		static std::stack<std::string> call_stack;
		
		GLFunctionLogger(const char* fn);
		~GLFunctionLogger();
	};
} // namespace gemini
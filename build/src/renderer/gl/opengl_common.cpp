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
#include <core/logging.h>
#include <runtime/filesystem.h>

#include <core/typedefs.h>

#include "renderer.h"
#include "opengl_common.h"
#include "render_utilities.h"

#include <json/json.h>

namespace renderer
{
	GLenum vertexbuffer_drawtype_to_gl_drawtype( renderer::VertexBufferDrawType type )
	{
		GLenum types[] = {
			GL_TRIANGLES, // draw_triangles
			GL_TRIANGLES, // draw_indexed_triangles
			GL_LINES,
			GL_POINTS
		};

		assert( type < renderer::DRAW_LIMIT );

		return types[ type ];
	} // vertexbuffer_drawtype_to_gl_drawtype

	GLenum vertexbuffer_buffertype_to_gl_buffertype( renderer::VertexBufferBufferType type )
	{
		GLenum types[] = {
			GL_STATIC_DRAW,
			GL_DYNAMIC_DRAW,
			GL_STREAM_DRAW
		};

		assert( type < renderer::BUFFER_LIMIT );

		return types[ type ];
	} // vertexbuffer_buffertype_to_gl_buffertype


	GLenum shaderobject_type_to_gl_shaderobjecttype( renderer::ShaderObjectType type )
	{
		GLenum types[] = {
			GL_VERTEX_SHADER,
			GL_FRAGMENT_SHADER,

			// not available in GLESv2
	//		GL_GEOMETRY_SHADER,
	//		GL_COMPUTE_SHADER
		};

		assert( type < renderer::SHADER_LIMIT );

		return types[ type ];
	}

	char * query_shader_info_log( GLObject handle )
	{
		int log_length = 0;
		char * logbuffer = 0;
		gl.GetShaderiv( handle, GL_INFO_LOG_LENGTH, &log_length );
		if ( log_length > 0 )
		{
			logbuffer = (char*)MEMORY_ALLOC(log_length+1, core::memory::global_allocator());
			memset( logbuffer, 0, log_length );

			gl.GetShaderInfoLog( handle, log_length, &log_length, logbuffer );
			if ( log_length > 0 )
			{
				return logbuffer;
			}
			else
			{
				MEMORY_DEALLOC(logbuffer, core::memory::global_allocator());
			}
		}

		return 0;
	} // query_shader_info_log

	char * query_program_info_log( GLObject handle )
	{
		int log_length = 0;
		char * logbuffer = 0;
		gl.GetProgramiv( handle, GL_INFO_LOG_LENGTH, &log_length );
		if ( log_length > 0 )
		{
			logbuffer = (char*)MEMORY_ALLOC(log_length+1, core::memory::global_allocator());
			memset( logbuffer, 0, log_length );

			gl.GetProgramInfoLog( handle, log_length, &log_length, logbuffer );
			if ( log_length > 0 )
			{
				return logbuffer;
			}
			else
			{
				MEMORY_DEALLOC(logbuffer, core::memory::global_allocator());
			}
		}

		return 0;
	} // query_program_info_log


	GLenum driver_state_to_gl_state( renderer::DriverState state )
	{
		GLenum state_list[] = {
			GL_CULL_FACE,
			GL_BLEND,
			GL_DEPTH_TEST,
		};

		return state_list[ state ];
	}

	GLenum convert_blendstate( renderer::RenderBlendType state )
	{
		GLenum glblend[] = {
			GL_ZERO,
			GL_ONE,
			GL_SRC_COLOR,
			GL_ONE_MINUS_SRC_COLOR,
			GL_DST_COLOR,
			GL_ONE_MINUS_DST_COLOR,
			GL_SRC_ALPHA,
			GL_ONE_MINUS_SRC_ALPHA,
			GL_ONE_MINUS_DST_ALPHA,
			GL_CONSTANT_COLOR,
			GL_ONE_MINUS_CONSTANT_COLOR,
			GL_CONSTANT_ALPHA,
			GL_ONE_MINUS_CONSTANT_ALPHA,
			GL_SRC_ALPHA_SATURATE,

	#if 0 // OpenGL 4.x +
			GL_SRC1_COLOR,
			GL_ONE_MINUS_SRC1_COLOR,
			GL_SRC1_ALPHA,
			GL_ONE_MINUS_SRC1_ALPHA
	#endif
		};

		return glblend[ state ];
	} // convert_blendstate

	GLenum cullmode_to_gl_cullmode( renderer::CullMode mode )
	{
		GLenum cullmode[] = {
			GL_FRONT,
			GL_BACK
		};

		return cullmode[ mode ];
	} // cullmode_to_gl_cullmode

	void state_op_enabledisable( renderer::DriverState state, core::util::MemoryStream & stream, renderer::IRenderDriver * driver )
	{
		GLenum gl_state;
		int enable = 0;

		stream.read( enable );

		// convert driver state to GL state
		gl_state = driver_state_to_gl_state(state);

		if ( enable )
		{
			gl.Enable( gl_state );
			gl.CheckError( "Enable" );
		}
		else
		{
			gl.Disable( gl_state );
			gl.CheckError( "Disable" );
		}
	} // state_op_enabledisable

	void state_op_depthmask( renderer::DriverState state, core::util::MemoryStream & stream, renderer::IRenderDriver * driver )
	{
		int enable = 0;
		GLboolean flag;

		stream.read( enable );
		flag = enable ? GL_TRUE : GL_FALSE;

		// set depth buffer enabled for writing flag
		gl.DepthMask( flag );
	} // state_op_depthmask




	gemgl_state_function operator_for_state( renderer::DriverState state )
	{
		// map driver state to operator function
		gemgl_state_function operator_map[] = {
			state_op_enabledisable,
			state_op_enabledisable,
			state_op_enabledisable,
			state_op_depthmask,
		};

		return operator_map[ state ];
	} // operator_for_state

	std::stack<std::string> GLFunctionLogger::call_stack;

	GLFunctionLogger::GLFunctionLogger(const char* fn) : function(fn)
	{
		GLFunctionLogger::call_stack.push(function);
		std::string prefix = "";
		for (size_t i = 0; i < GLFunctionLogger::call_stack.size(); ++i)
		{
			prefix += "\t";
		}
		LOGV("-> %s%s\n", prefix.c_str(), function);
	}

	GLFunctionLogger::~GLFunctionLogger()
	{
		const std::string& call = GLFunctionLogger::call_stack.top();
		GLFunctionLogger::call_stack.pop();
		std::string prefix = "";
		for (size_t i = 0; i < GLFunctionLogger::call_stack.size(); ++i)
		{
			prefix += "\t";
		}
		LOGV(".. %s%s\n", prefix.c_str(), call.c_str());
	}
} // namespace renderer



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
#include <platform/typedefs.h>
#include <slim/xlog.h>

#include "renderer.h"
#include "opengl_common.h"

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
		logbuffer = (char*)ALLOC( log_length+1 );
		memset( logbuffer, 0, log_length );
		
		gl.GetShaderInfoLog( handle, log_length, &log_length, logbuffer );
		if ( log_length > 0 )
		{
			return logbuffer;
		}
		else
		{
			DEALLOC(logbuffer);
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
		logbuffer = (char*)ALLOC( log_length+1 );
		memset( logbuffer, 0, log_length );
		
		gl.GetProgramInfoLog( handle, log_length, &log_length, logbuffer );
		if ( log_length > 0 )
		{
			return logbuffer;
		}
		else
		{
			DEALLOC(logbuffer);
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

void state_op_enabledisable( renderer::DriverState state, util::MemoryStream & stream, renderer::IRenderDriver * driver )
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

void state_op_depthmask( renderer::DriverState state, util::MemoryStream & stream, renderer::IRenderDriver * driver )
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
	for (int i = 0; i < GLFunctionLogger::call_stack.size(); ++i)
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
	for (int i = 0; i < GLFunctionLogger::call_stack.size(); ++i)
	{
		prefix += "\t";
	}
	LOGV(".. %s%s\n", prefix.c_str(), call.c_str());
}
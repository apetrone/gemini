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
#include "typedefs.h"
#include "renderer.hpp"
#include "opengl_common.hpp"

GLenum vertexbuffer_drawtype_to_gl_drawtype( renderer::VertexBufferDrawType type )
{
	GLenum types[] = {
		GL_TRIANGLES,
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
		GL_GEOMETRY_SHADER,
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
		logbuffer = (char*)memory::allocator().allocate(log_length+1);
		memset( logbuffer, 0, log_length );
		
		gl.GetShaderInfoLog( handle, log_length, &log_length, logbuffer );
		if ( log_length > 0 )
		{
			return logbuffer;
		}
		else
		{
			memory::allocator().deallocate(logbuffer);
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
		logbuffer = (char*)memory::allocator().allocate(log_length+1);
		memset( logbuffer, 0, log_length );
		
		gl.GetProgramInfoLog( handle, log_length, &log_length, logbuffer );
		if ( log_length > 0 )
		{
			return logbuffer;
		}
		else
		{
			memory::allocator().deallocate(logbuffer);
		}
	}
	
	return 0;
} // query_program_info_log

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
} // VertexBufferDrawType_OpenGLDrawType

GLenum vertexbuffer_buffertype_to_gl_buffertype( renderer::VertexBufferBufferType type )
{
	GLenum types[] = {
		GL_STATIC_DRAW,
		GL_DYNAMIC_DRAW,
		GL_STREAM_DRAW
	};
	
	assert( type < renderer::BUFFER_LIMIT );
	
	return types[ type ];
} // VertexBufferBufferType_OpenGLBufferType
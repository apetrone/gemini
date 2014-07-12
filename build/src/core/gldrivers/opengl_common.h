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

#include <gemini/util/stackstring.h>

#include "renderer.h"

#define FAIL_IF_GLERROR( error ) if ( error != GL_NO_ERROR ) { return false; }

#include "gemgl.h" // for GLObject


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

typedef void (*gemgl_state_function)(renderer::DriverState, MemoryStream &, renderer::IRenderDriver *);

gemgl_state_function operator_for_state( renderer::DriverState state );

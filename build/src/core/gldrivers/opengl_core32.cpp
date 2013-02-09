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
#include "log.h"
#include "gldrivers/opengl_core32.hpp"
#include "gemgl.hpp"
#include "opengl_common.hpp"

using namespace renderer;

GLCore32::GLCore32()
{
	LOGV( "GLCore32 instanced.\n" );
	gemgl_config config;
	config.type = renderer::OpenGL;
	config.major_version = 3;
	config.minor_version = 2;
	
	gemgl_startup( gl, config );
}

GLCore32::~GLCore32()
{
	LOGV( "GLCore32 shutting down.\n" );
	gemgl_shutdown( gl );
}

void GLCore32::run_command( renderer::DriverCommandType command, MemoryStream & stream )
{
	switch( command )
	{
		case DC_CLEARCOLOR:
		{
			float color[4];
			stream.read( color, 4*sizeof(float) );
			gl.ClearColor( color[0], color[1], color[2], color[3] );
			break;
		}

		case DC_CLEAR:
		{
			unsigned int bits;
			stream.read(bits);
			gl.Clear( bits );
			break;
		}
		
		case DC_VIEWPORT:
		{
			int x, y, width, height;
			stream.read(x);
			stream.read(y);
			stream.read(width);
			stream.read(height);
			gl.Viewport( x, y, width, height );
			break;
		}
		
		default: break;
	}
}

void GLCore32::post_command( renderer::DriverCommandType command, MemoryStream & stream )
{
	
}
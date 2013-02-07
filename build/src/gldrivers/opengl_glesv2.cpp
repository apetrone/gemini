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
#include "gldrivers/opengl_glesv2.hpp"
#include "gemgl.h"
#include "opengl_common.hpp"

using namespace renderer;

GLESv2::GLESv2()
{
	LOGV( "GLESv2 instanced.\n" );
	gemgl_startup( &gl, GEMGL_CORE_32 );
}

GLESv2::~GLESv2()
{
	LOGV( "GLESv2 shutting down.\n" );
	gemgl_shutdown( &gl );
}

void GLESv2::run_command( renderer::DriverCommandType command, MemoryStream & stream )
{
	switch( command )
	{
		case DC_CLEARCOLOR:
		{
			float r, g, b, a;
			stream.read(r);
			stream.read(g);
			stream.read(b);
			stream.read(a);
			gl.ClearColor( r, g, b, a );
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

void GLESv2::post_command( renderer::DriverCommandType command, MemoryStream & stream )
{
	
}
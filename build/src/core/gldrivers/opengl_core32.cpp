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
#include "image.hpp"

#define FAIL_IF_GLERROR( error ) if ( error != GL_NO_ERROR ) { return false; }

using namespace renderer;


// utility functions
GLenum image_to_source_format( int num_channels )
{
	if ( num_channels == 3 )
	{
		return GL_RGB;
	}
	else if ( num_channels == 4 )
	{
		return GL_RGBA;
	}
	
	return GL_RGBA;
} // image_to_source_format

GLenum image_to_internal_format( unsigned int image_flags )
{
	//GLenum internalFormat = GL_SRGB8;
	if ( image_flags & image::F_RGBA )
	{
		return GL_RGBA;
	}
	else if ( image_flags & image::F_ALPHA )
	{
		return GL_ALPHA;
	}
	
	return GL_RGBA;
} // image_to_internal_format



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


bool GLCore32::upload_texture_2d( renderer::TextureParameters & parameters )
{
	GLenum source_format = image_to_source_format( parameters.channels );
	GLenum internal_format = image_to_internal_format( parameters.image_flags );
	GLenum error = GL_NO_ERROR;
	
	// bind the texture so it is active
	gl.BindTexture( GL_TEXTURE_2D, parameters.texture_id );
	error = gl.CheckError( "upload_texture_2d - BindTexture" );
	FAIL_IF_GLERROR(error);
	
	if ( !this->is_texture(parameters) )
	{
		LOGE( "request to upload_texture_2d failed: not an image!\n" );
		return false;
	}
		
	if ( parameters.image_flags & image::F_CLAMP )
	{
		gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		error = gl.CheckError( "upload_texture_2d - TexParameteri GL_TEXTURE_WRAP_S GL_CLAMP_TO_EDGE" );
		FAIL_IF_GLERROR(error);
		
		gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		error = gl.CheckError( "upload_texture_2d - TexParameteri GL_TEXTURE_WRAP_T GL_CLAMP_TO_EDGE" );
		FAIL_IF_GLERROR(error);
	}
	else
	{
		gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		error = gl.CheckError( "upload_texture_2d - TexParameteri GL_TEXTURE_WRAP_S GL_REPEAT" );
		FAIL_IF_GLERROR(error);
		
		gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		error = gl.CheckError( "upload_texture_2d - TexParameteri GL_TEXTURE_WRAP_T GL_REPEAT" );
		FAIL_IF_GLERROR(error);
	}

	//gl.TexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4 );
	
	gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	error = gl.CheckError( "upload_texture_2d - GL_TEXTURE_MIN_FILTER" );
	FAIL_IF_GLERROR(error);
	
	gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	error = gl.CheckError( "upload_texture_2d - GL_TEXTURE_MAG_FILTER" );
	FAIL_IF_GLERROR(error);

	gl.TexImage2D( GL_TEXTURE_2D, 0, internal_format, parameters.width, parameters.height, 0, source_format, GL_UNSIGNED_BYTE, parameters.pixels );
	error = gl.CheckError( "upload_texture_2d - glTexImage2D" );
	FAIL_IF_GLERROR(error);
	
	gl.GenerateMipmap( GL_TEXTURE_2D );
	error = gl.CheckError( "upload_texture_2d - GenerateMipmap" );
	FAIL_IF_GLERROR(error);


	gl.BindTexture( GL_TEXTURE_2D, 0 );
	error = gl.CheckError( "upload_texture_2d - (un)BindTexture" );
	FAIL_IF_GLERROR(error);

	return true;
} // upload_texture_2d

bool GLCore32::generate_texture( renderer::TextureParameters & parameters )
{
	gl.GenTextures( 1, &parameters.texture_id );
	GLenum error = gl.CheckError( "generate_texture" );
	return (error == GL_NO_ERROR);
} // generate_texture

bool GLCore32::destroy_texture( renderer::TextureParameters & parameters )
{
	gl.DeleteTextures( 1, &parameters.texture_id );
	GLenum error = gl.CheckError( "destroy_texture" );
	return (error == GL_NO_ERROR);
} // destroy_texture

bool GLCore32::is_texture( renderer::TextureParameters & parameters )
{
	return gl.IsTexture( parameters.texture_id );
} // is_texture
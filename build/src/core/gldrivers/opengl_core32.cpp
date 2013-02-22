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


struct GL32GeometryStream : public GeometryStream
{
	GLuint vao;
	GLuint vbo[2];
};


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

void GLCore32::render_font( int x, int y, renderer::Font & font, const char * utf8_string, const Color & color )
{
	/*
	1. update buffers
	2. glViewport
	3. enable blending: GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
	4. activate font shader
	5. set relevant uniforms (modelview, projection, etc)
	6. activate font texture
	7. bind font texture
	8. draw elements of buffer
	9. disable blending
	10. deactivate shader
	11. unbind texture
	12. reset buffer
	*/
} // render_font

renderer::GeometryStream * GLCore32::geometrystream_create( renderer::VertexDescriptor & descriptor, GeometryStreamDrawType draw_type, GeometryStreamBufferType buffer_type, unsigned int vertex_size, unsigned int max_vertices, unsigned int max_indices )
{
	GLenum gl_buffer_type = GeometryStreamBufferType_OpenGLBufferType( buffer_type );
	
	GL32GeometryStream * stream = ALLOC(GL32GeometryStream);
	assert( stream != 0 );
	
	// initial values for stream
	stream->vao = 0;
	stream->vbo[0] = 0;
	stream->vbo[1] = 0;
	
	gl.GenVertexArrays( 1, &stream->vao );
	gl.BindVertexArray( stream->vao );
	
	gl.GenBuffers( 1, stream->vbo );
	gl.BindBuffer( GL_ARRAY_BUFFER, stream->vbo[0] );
	gl.CheckError( "BindBuffer" );
	
	gl.BufferData( GL_ARRAY_BUFFER, vertex_size * max_vertices, 0, gl_buffer_type );
	gl.CheckError( "BufferData" );

	if ( max_indices > 0 )
	{
		gl.GenBuffers( 1, &stream->vbo[1] );
		gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, stream->vbo[1] );
		gl.CheckError( "BindBuffer" );
		
		gl.BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * max_indices, 0, gl_buffer_type );
		gl.CheckError( "BufferData" );
	}

	// reset the descriptor and iterate over the items to setup the vertex attributes
	descriptor.reset();
	GLenum attrib_type = GL_INVALID_ENUM;
	VertexDescriptorType desc_type;
	unsigned int attribID = 0;
	unsigned int attribSize = 0;
	unsigned int num_elements = 0;
	unsigned int normalized = 0;
	unsigned int offset = 0;
	
	for( unsigned int i = 0; i < descriptor.attribs; ++i )
	{
		desc_type = descriptor.description[i];
		if ( desc_type == VD_FLOAT2 )
		{
			attrib_type = GL_FLOAT;
			normalized = GL_FALSE;
		}
		else if ( desc_type == VD_FLOAT3 )
		{
			attrib_type = GL_FLOAT;
			normalized = GL_FALSE;
		}
		else if ( desc_type == VD_UNSIGNED_INT )
		{
			attrib_type = GL_UNSIGNED_INT;
			normalized = GL_FALSE;
		}
		else if ( desc_type == VD_UNSIGNED_BYTE3 )
		{
			attrib_type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
		}
		else if ( desc_type == VD_UNSIGNED_BYTE4 )
		{
			attrib_type = GL_UNSIGNED_BYTE;
			normalized = GL_TRUE;
		}
		
		num_elements = VertexDescriptor::elements[ desc_type ];
		attribSize = VertexDescriptor::size[ desc_type ];
		gl.VertexAttribPointer( attribID, num_elements, attrib_type, normalized, vertex_size, (void*)offset );
		gl.CheckError( "VertexAttribPointer" );
		
		gl.EnableVertexAttribArray( attribID );
		gl.CheckError( "EnableVertexAttribArray" );
				
		offset += attribSize;
		++attribID;
	}

	gl.BindBuffer( GL_ARRAY_BUFFER, 0 );
	gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	gl.BindVertexArray( 0 );
		
	return stream;
} // geometrystream_create

void GLCore32::geometrystream_destroy( renderer::GeometryStream * geometrystream )
{
	GL32GeometryStream * stream = (GL32GeometryStream*)geometrystream;
	
	if ( stream->vao != 0 )
	{
		gl.DeleteVertexArrays( 1, &stream->vao );
	}
	
	if ( stream->vbo[0] != 0 )
	{
		gl.DeleteBuffers( 1, stream->vbo );
	}
	
	if ( stream->vbo[1] != 0 )
	{
		gl.DeleteBuffers( 1, &stream->vbo[1] );
	}
	
	
	DEALLOC(GL32GeometryStream, stream);
} // geometrystream_destroy
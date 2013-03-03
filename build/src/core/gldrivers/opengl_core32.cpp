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

#include "image.hpp"

#include "mathlib.h"
#include "memorystream.hpp"



#define FAIL_IF_GLERROR( error ) if ( error != GL_NO_ERROR ) { return false; }
using namespace renderer;


struct GL32VertexBuffer : public VertexBuffer
{
	GLuint vao;
	GLuint vbo[2];
	GLenum gl_buffer_type;
	GLenum gl_draw_type;
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


GLenum driver_state_to_gl_state( DriverState state )
{
	switch( state )
	{
		case STATE_BLEND: return GL_BLEND;
		default: return GL_FALSE;
	}
}

GLenum convert_blendstate( RenderBlendType state )
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


GLCore32::GLCore32()
{
	LOGV( "GLCore32 instanced.\n" );
	gemgl_config config;
	config.type = renderer::OpenGL;
	config.major_version = 3;
	config.minor_version = 2;
	
	gemgl_startup( gl, config );
	last_shader = 0;
}

GLCore32::~GLCore32()
{
	LOGV( "GLCore32 shutting down.\n" );
	gemgl_shutdown( gl );
}



void c_shader( MemoryStream & stream, GLCore32 & renderer )
{
	ShaderProgram shader_program;
	stream.read( shader_program.object );
	
	renderer.shaderprogram_activate( shader_program );
}

void c_uniform_matrix4( MemoryStream & stream, GLCore32 & renderer )
{	
	int uniform_location;
	glm::mat4 * matrix = 0;
	stream.read( matrix );
	stream.read( uniform_location );
	
	gl.UniformMatrix4fv( uniform_location, 1, GL_FALSE, glm::value_ptr(*matrix) );
	gl.CheckError( "uniform matrix 4" );
}

void c_uniform_sampler2d( MemoryStream & stream, GLCore32 & renderer )
{
	//	LogMsg( "R_UNIFORM_SAMPLER_2D\n" );
	//	int texture_unit = bitstream_readint( stream );
	//	int texture_id = bitstream_readint( stream );
	//	int uniform_location = bitstream_readint( stream );
	
	
	int texture_unit;
	int texture_id;
	int uniform_location;
	
	stream.read( texture_unit );
	stream.read( texture_id );
	stream.read( uniform_location );
	
	//	LogMsg( "texture_unit: %i, texture_id: %i, location: %i\n", texture_unit, texture_id, uniform_location );
	
	//	if ( last_texture[ texture_unit ] != texture_id )
	{
		gl.ActiveTexture( GL_TEXTURE0+texture_unit );
		gl.CheckError( "ActiveTexture" );
		
		gl.BindTexture( GL_TEXTURE_2D, texture_id );
		gl.CheckError( "BindTexture: GL_TEXTURE_2D" );
		
		gl.Uniform1i( uniform_location, texture_unit );
		gl.CheckError( "uniform1i" );
		
		//		++texture_switches;
		//		last_texture[ texture_unit ] = texture_id;
	}
}

void c_clear( MemoryStream & stream, GLCore32 & renderer )
{
	unsigned int bits;
	stream.read(bits);
	gl.Clear( bits | GL_DEPTH_BUFFER_BIT );
}

void c_clearcolor( MemoryStream & stream, GLCore32 & renderer )
{
	float color[4];
	stream.read( color, 4*sizeof(float) );
	gl.ClearColor( color[0], color[1], color[2], color[3] );
}

void c_cleardepth( MemoryStream & stream, GLCore32 & renderer )
{
	float value;
	stream.read( value );
	glClearDepth( value );
}

void c_viewport( MemoryStream & stream, GLCore32 & renderer )
{
	int x, y, width, height;
	//			stream.read(x);
	//			stream.read(y);
	//			stream.read(width);
	//			stream.read(height);
	stream.read( &x, 4 );
	stream.read( &y, 4 );
	stream.read( &width, 4 );
	stream.read( &height, 4 );
	gl.Viewport( x, y, width, height );
}


void c_state( MemoryStream & stream, GLCore32 & renderer )
{
	// state change
	DriverState driver_state;
	
	GLenum state;
	int enable = 0;
	
	stream.read( driver_state );
	stream.read( enable );
	
	// convert driver state to GL state
	state = driver_state_to_gl_state( driver_state );
	
	if ( enable )
	{
		gl.Enable( state );
	}
	else
	{
		gl.Disable( state );
	}
}

void c_blendfunc( MemoryStream & stream, GLCore32 & renderer )
{
	RenderBlendType render_blendstate_source, render_blendstate_destination;
		
	stream.read( render_blendstate_source );
	stream.read( render_blendstate_destination );

	GLenum source = convert_blendstate( render_blendstate_source );
	GLenum destination = convert_blendstate( render_blendstate_destination );
	
	gl.BlendFunc( source, destination );
}


void c_noop( MemoryStream & stream, GLCore32 & renderer )
{
}

typedef void (*render_command_function)( MemoryStream & stream, GLCore32 & renderer );


render_command_function commands[] = {
	c_shader, // shader
	c_uniform_matrix4, // uniform_matrix4
	c_noop, // uniform1i
	c_noop, // uniform3f
	c_noop, // uniform4f
	c_uniform_sampler2d, // uniform_sampler_2d
	c_noop, // uniform_sampler_cube

	c_clear, // clear
	c_clearcolor, // clearcolor
	c_cleardepth, // cleardepth
	c_viewport, // viewport
	
	c_noop, // drawcall
	c_noop, // scissor
	c_state, // state
	c_blendfunc, // blendfunc
};


void GLCore32::run_command( renderer::DriverCommandType command, MemoryStream & stream )
{
	commands[ command ]( stream, *this );
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

renderer::VertexBuffer * GLCore32::vertexbuffer_create( renderer::VertexDescriptor & descriptor, VertexBufferDrawType draw_type, VertexBufferBufferType buffer_type, unsigned int vertex_size, unsigned int max_vertices, unsigned int max_indices )
{
	GL32VertexBuffer * stream = CREATE(GL32VertexBuffer);
	assert( stream != 0 );
	
	// initial values for stream
	stream->vao = 0;
	stream->vbo[0] = 0;
	stream->vbo[1] = 0;
	stream->gl_draw_type = vertexbuffer_drawtype_to_gl_drawtype( draw_type );
	stream->gl_buffer_type = vertexbuffer_buffertype_to_gl_buffertype( buffer_type );
		
	gl.GenVertexArrays( 1, &stream->vao );
	gl.CheckError( "GenVertexArrays" );
	
	gl.BindVertexArray( stream->vao );
	gl.CheckError( "BindVertexArray" );
	
	gl.GenBuffers( 1, stream->vbo );
	gl.BindBuffer( GL_ARRAY_BUFFER, stream->vbo[0] );
	gl.CheckError( "BindBuffer" );
	
	gl.BufferData( GL_ARRAY_BUFFER, vertex_size * max_vertices, 0, stream->gl_buffer_type );
	gl.CheckError( "BufferData" );

	if ( max_indices > 0 )
	{
		gl.GenBuffers( 1, &stream->vbo[1] );
		gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, stream->vbo[1] );
		gl.CheckError( "BindBuffer" );
		
		gl.BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * max_indices, 0, stream->gl_buffer_type );
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
} // vertexbuffer_create

void GLCore32::vertexbuffer_destroy( renderer::VertexBuffer * vertexbuffer )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	
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
	
	
	DESTROY(GL32VertexBuffer, stream);
} // vertexbuffer_destroy

void GLCore32::vertexbuffer_bufferdata( VertexBuffer * vertexbuffer, unsigned int vertex_stride, unsigned int vertex_count, VertexType * vertices, unsigned int index_count, IndexType * indices )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	assert( stream != 0 );
	
	gl.BindVertexArray( stream->vao );
	
	gl.BindBuffer( GL_ARRAY_BUFFER, stream->vbo[0] );
	gl.CheckError( "BindBuffer GL_ARRAY_BUFFER" );
	gl.BufferData( GL_ARRAY_BUFFER, vertex_stride * vertex_count, vertices, stream->gl_buffer_type );
	
	if ( stream->vbo[1] != 0 )
	{
		gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, stream->vbo[1] );
		gl.CheckError( "BindBuffer GL_ELEMENT_ARRAY_BUFFER" );
		gl.BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * index_count, indices, stream->gl_buffer_type );
	}
	
	gl.BindVertexArray( 0 );
	gl.BindBuffer( GL_ARRAY_BUFFER, 0 );
	gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


void GLCore32::vertexbuffer_draw_indices( renderer::VertexBuffer * vertexbuffer, unsigned int num_indices )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	assert( stream != 0 );
	gl.BindVertexArray( stream->vao );
	gl.CheckError( "BindVertexArray" );
	gl.DrawElements( stream->gl_draw_type, num_indices, GL_UNSIGNED_INT, 0 );
	gl.CheckError( "DrawElements" );
	gl.BindVertexArray( 0 );
}

void GLCore32::vertexbuffer_draw( renderer::VertexBuffer * vertexbuffer, unsigned int num_vertices )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	assert( stream != 0 );
	
	gl.BindVertexArray( stream->vao );
	gl.CheckError( "BindVertexArray" );
	
	gl.DrawArrays( stream->gl_draw_type, 0, num_vertices );
	gl.CheckError( "DrawArrays" );
	gl.BindVertexArray( 0 );
}

///////////////////////////////
// Shaders
// ---------------------------------------

//#define SHADER_DEBUG( fmt, ... ) (void(0))
#define SHADER_DEBUG LOGV

renderer::ShaderObject GLCore32::shaderobject_create( renderer::ShaderObjectType shader_type )
{
	GLenum type = shaderobject_type_to_gl_shaderobjecttype( shader_type );
	ShaderObject object;
	
	object.flags = 0;
	object.shader_id = gl.CreateShader( type );
	gl.CheckError( "CreateShader" );

	return object;
}

bool GLCore32::shaderobject_compile( renderer::ShaderObject shader_object, const char * shader_source, const char * preprocessor_defines, const char * version )
{
	const int MAX_SHADER_SOURCES = 3;
	GLint is_compiled = 0;
	const char * shaderSource[ MAX_SHADER_SOURCES ] = { version, preprocessor_defines, shader_source };
	
	// provide the sources
	gl.ShaderSource( shader_object.shader_id, MAX_SHADER_SOURCES, (GLchar**)shaderSource, 0 );
	gl.CheckError( "ShaderSource" );
	
	gl.CompileShader( shader_object.shader_id );
	gl.CheckError( "CompileShader" );
	gl.GetShaderiv( shader_object.shader_id, GL_COMPILE_STATUS, &is_compiled );
	if ( !is_compiled )
	{
		LOGE( "Error compiling shader!\n" );
		char * logbuffer = query_shader_info_log( shader_object.shader_id );
		if ( logbuffer )
		{
			LOGW( "Shader Info Log:\n" );
			LOGW( "%s\n", logbuffer );
			DEALLOC(logbuffer);
		}
	}

	return true;
}

void GLCore32::shaderobject_destroy( renderer::ShaderObject shader_object )
{
	gl.DeleteShader( shader_object.shader_id );
	gl.CheckError( "DeleteShader" );
}

renderer::ShaderProgram GLCore32::shaderprogram_create( renderer::ShaderParameters & parameters )
{
	ShaderProgram program;
	program.object = gl.CreateProgram();
	gl.CheckError( "CreateProgram" );
	
	if ( !gl.IsProgram( program.object ) )
	{
		LOGE("generated object is NOT a program!\n" );
	}
	else
	{
		LOGV( "created program: %i\n", program.object );
	}
	
	return program;
}

void GLCore32::shaderprogram_destroy( renderer::ShaderProgram program )
{
	if ( program.object != 0 )
	{
		gl.DeleteProgram( program.object );
		gl.CheckError( "DeleteProgram" );
	}
}

void GLCore32::shaderprogram_attach( renderer::ShaderProgram shader_program, renderer::ShaderObject shader_object )
{
	gl.AttachShader( shader_program.object, shader_object.shader_id );
	gl.CheckError( "AttachShader" );
}

void GLCore32::shaderprogram_bind_attributes( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters )
{
	gl.BindFragDataLocation( shader_program.object, 0, parameters.frag_data_location);
	gl.CheckError( "BindFragDataLocation" );

	for( int i = 0; i < parameters.total_attributes; ++i )
	{
		ShaderKeyValuePair * keyvalue = &parameters.attributes[i];
		SHADER_DEBUG( "BindAttribLocation -> %s to %i\n", keyvalue->first, keyvalue->second );
		gl.BindAttribLocation( shader_program.object, keyvalue->second, keyvalue->first );
		gl.CheckError( xstr_format( "BindAttribLocation: %s", keyvalue->first ));
	}
}

void GLCore32::shaderprogram_bind_uniforms( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters )
{
	// ensure this is the active shader before binding uniforms
	this->shaderprogram_activate( shader_program );

	// fetch uniforms from the shader
	for( int uniform_id = 0; uniform_id < parameters.total_uniforms; ++uniform_id )
	{
		ShaderKeyValuePair * keyvalue = &parameters.uniforms[ uniform_id ];
		
		keyvalue->second = gl.GetUniformLocation( shader_program.object, keyvalue->first );
		SHADER_DEBUG( "GetUniformLocation: \"%s\" -> %i\n", keyvalue->first, keyvalue->second );
		gl.CheckError( "GetUniformLocation" );
		
		if ( keyvalue->second == -1 )
		{
			LOGE( "GetUniformLocation FAILED for \"%s\"\n", keyvalue->first );
		}
	}
}

void GLCore32::shaderprogram_link_and_validate( renderer::ShaderProgram shader_program )
{
	gl.LinkProgram( shader_program.object );
	gl.CheckError( "LinkProgram" );
	
	GLint link_status;
	gl.GetProgramiv( shader_program.object, GL_LINK_STATUS, &link_status );
	gl.CheckError( "GetProgramiv" );
	
	if ( !link_status )
	{
		LOGE( "Error linking program!\n" );
		char * logbuffer = query_program_info_log( shader_program.object );
		if ( logbuffer )
		{
			LOGW( "Program Info Log:\n" );
			LOGW( "%s\n", logbuffer );
			DEALLOC(logbuffer);
		}
		
		assert( link_status == 1 );
	}
	
#if 0
	gl.ValidateProgram( shader_program.object );
	int validate_status;
	gl.GetProgramiv( shader_program.object, GL_VALIDATE_STATUS, &validate_status );
	if ( !validate_status )
	{
		LOGE( "Program validation failed; last operation unsuccessful.\n" );
		char * logbuffer = query_program_info_log( shader_program.object );
		if ( logbuffer )
		{
			LOGW( "Program Info Log:\n" );
			LOGW( "%s\n", logbuffer );
			DEALLOC(logbuffer);
		}
		
		assert( validate_status == 1 );
	}
#endif

}

void GLCore32::shaderprogram_activate( renderer::ShaderProgram shader_program )
{
//	if ( shader_program.object != last_shader )
	{
		gl.UseProgram( shader_program.object );
		gl.CheckError( "UseProgram" );
//		last_shader = shader_program.object;
//		++shader_changes;
	}
}

void GLCore32::shaderprogram_deactivate( renderer::ShaderProgram shader_program )
{
//	last_shader = 0;
	gl.UseProgram( 0 );
}



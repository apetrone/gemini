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

#include "opengl_core32.h"

#include "image.h"

#include <core/mathlib.h>
#include <core/datastream.h>

// utility functions
GLenum image_source_format( int num_channels )
{
	if ( num_channels == 3 )
	{
		return GL_RGB;
	}
	else if ( num_channels == 4 )
	{
		return GL_RGBA;
	}
	else if ( num_channels == 1 )
	{
		return GL_RED;
	}
	
	return GL_RGBA;
} // image_source_format

GLenum image_internal_format( unsigned int image_flags )
{
	//GLenum internalFormat = GL_SRGB8;
	if ( image_flags & image::F_RGBA )
	{
		return GL_RGBA;
	}
	else if ( image_flags & image::F_ALPHA )
	{
		return GL_RED;
	}
	
	return GL_RGBA;
} // image_internal_format

GLenum srgb_image_to_internal_format(unsigned int image_flags)
{
	if (image_flags & image::F_RGBA)
	{
		return GL_SRGB8_ALPHA8;
	}
	else if (image_flags & image::F_ALPHA)
	{
		return GL_RED;
	}
	
	return GL_SRGB8_ALPHA8;
}

#define FAIL_IF_GLERROR( error ) if ( error != GL_NO_ERROR ) { return false; }
using namespace renderer;


enum GL32VAOType
{
//	VAO_POSITIONS_ONLY,
	VAO_INTERLEAVED,
	
	VAO_LIMIT = 1
};

enum GL32VBOType
{
	VBO_LIMIT = 2
};

struct GL32VertexBuffer : public VertexBuffer
{
	GLuint vao[ VAO_LIMIT ];
	GLuint vbo[ VBO_LIMIT ];
	GLenum gl_buffer_type;
	GLenum gl_draw_type;
	unsigned int vertex_stride;
	
	GL32VertexBuffer()
	{
		for( unsigned int i = 0; i < VAO_LIMIT; ++i )
		{
			vao[i] = 0;
		}
		
		for( unsigned int i = 0; i < VBO_LIMIT; ++i )
		{
			vbo[i] = 0;
		}
		
		gl_draw_type = 0;
		gl_buffer_type = 0;
		
		vertex_stride = 0;
	}
	
	void allocate( renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type )
	{
		vao[ VAO_INTERLEAVED ] = 0;
		memset( vbo, 0, sizeof(GLuint) );
		gl_draw_type = vertexbuffer_drawtype_to_gl_drawtype( draw_type );
		gl_buffer_type = vertexbuffer_buffertype_to_gl_buffertype( buffer_type );
	}
	
	
	void static_setup( renderer::VertexDescriptor & descriptor, unsigned int vertex_stride, unsigned int max_vertices, unsigned int max_indices )
	{
		this->vertex_stride = vertex_stride;
		
		gl.GenVertexArrays( VAO_LIMIT, this->vao );
		gl.CheckError( "GenVertexArrays" );
		
		gl.GenBuffers( VBO_LIMIT, this->vbo );
		gl.CheckError( "GenBuffers" );
		
		gl.BindVertexArray( this->vao[ VAO_INTERLEAVED ] );
		gl.CheckError( "BindVertexArray" );
				
		gl.BindBuffer( GL_ARRAY_BUFFER, this->vbo[0] );
		gl.CheckError( "BindBuffer" );
		
		gl.BufferData( GL_ARRAY_BUFFER, vertex_stride * max_vertices, 0, this->gl_buffer_type );
		gl.CheckError( "BufferData" );
		
		if ( max_indices > 0 )
		{
			gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, this->vbo[1] );
			gl.CheckError( "BindBuffer" );
			
			gl.BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * max_indices, 0, this->gl_buffer_type );
			gl.CheckError( "BufferData" );
		}
		
		// reset the descriptor and iterate over the items to setup the vertex attributes
		descriptor.reset();
		
		VertexDescriptorType desc_type;
		unsigned int attribID = 0;
		unsigned int attribSize = 0;
		unsigned int num_elements = 0;
		unsigned int normalized = 0;
		size_t offset = 0;
		
		for( unsigned int i = 0; i < descriptor.attribs; ++i )
		{
			GLenum attrib_type = GL_INVALID_ENUM;
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
			else if ( desc_type == VD_FLOAT4 )
			{
				attrib_type = GL_FLOAT;
				normalized = GL_FALSE;
			}
			else if ( desc_type == VD_INT4 )
			{
				attrib_type = GL_INT;
				normalized = GL_TRUE;
			}
			else if ( desc_type == VD_UNSIGNED_INT )
			{
				attrib_type = GL_UNSIGNED_INT;
				normalized = GL_TRUE;
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
			
			assert(attrib_type != GL_INVALID_ENUM);
			
			num_elements = VertexDescriptor::elements[ desc_type ];
			attribSize = VertexDescriptor::size[ desc_type ];
			
			gl.EnableVertexAttribArray( attribID );
			gl.CheckError( "EnableVertexAttribArray" );
			
			gl.VertexAttribPointer( attribID, num_elements, attrib_type, normalized, vertex_stride, (void*)offset );
			gl.CheckError( "VertexAttribPointer" );

			offset += attribSize;
			++attribID;
		}
		
		gl.BindVertexArray( 0 );
		gl.BindBuffer( GL_ARRAY_BUFFER, 0 );
		gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}
	
	void upload_interleaved_data( const GLvoid * data, unsigned int vertex_count )
	{
		this->vertex_count = vertex_count;
		
		gl.BindBuffer( GL_ARRAY_BUFFER, this->vbo[0] );
		gl.CheckError( "BindBuffer GL_ARRAY_BUFFER" );
		
		gl.BufferData( GL_ARRAY_BUFFER, vertex_stride * vertex_count, data, this->gl_buffer_type );
		gl.CheckError( "BufferData GL_ARRAY_BUFFER" );
	}
	
	void upload_index_array( IndexType * indices, unsigned int index_count )
	{
		if ( this->vbo[1] != 0 )
		{
			this->index_count = index_count;
			gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, this->vbo[1] );
			gl.CheckError( "BindBuffer GL_ELEMENT_ARRAY_BUFFER" );
			
			gl.BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * index_count, indices, this->gl_buffer_type );
			gl.CheckError( "BufferData - GL_ELEMENT_ARRAY_BUFFER" );
		}
	}
};

struct GL32Texture : public renderer::Texture
{
	unsigned int texture_id;
	GLenum texture_type;
	uint8_t unpack_alignment;
	
	GL32Texture()
	{
		glGenTextures(1, &texture_id);
		
		// setup sane defaults
		texture_type = GL_TEXTURE_2D;
		unpack_alignment = 4;
	}
	
	~GL32Texture()
	{
		glDeleteTextures(1, &texture_id);
	}
	
	void bind(bool activate = true)
	{
		GLuint id = activate ? texture_id : 0;
		gl.BindTexture(texture_type, id);
	}
	
	void unbind()
	{
		bind(false);
	}
		
	void set_parameters(image::Image& image)
	{
		GLenum wrap_type = GL_REPEAT;
		// setup texture wrapping
		if (image.flags & image::F_CLAMP)
		{
			wrap_type = GL_CLAMP_TO_EDGE;
		}
		else if (image.flags & image::F_CLAMP_BORDER)
		{
			wrap_type = GL_CLAMP_TO_BORDER;
					
			float border[] = {1, 1, 1, 1};
			glTexParameterfv(texture_type, GL_TEXTURE_BORDER_COLOR, border);
		}
		
		gl.TexParameteri(texture_type, GL_TEXTURE_WRAP_S, wrap_type);
		gl.TexParameteri(texture_type, GL_TEXTURE_WRAP_T, wrap_type);
		
		GLenum min_filter = GL_NEAREST;
		GLenum mag_filter = GL_NEAREST;
		
		if (image.filter == image::FILTER_LINEAR)
		{
			min_filter = GL_LINEAR_MIPMAP_NEAREST;
			mag_filter = GL_NEAREST;
		}
		else if (image.filter == image::FILTER_LINEAR_MIPMAP)
		{
			min_filter = GL_LINEAR_MIPMAP_LINEAR;
			mag_filter = GL_LINEAR;
		}
		
		// set filtering
		gl.TexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, min_filter);
		gl.TexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, mag_filter);
		
		
		//gl.TexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4 );
	}
	
//	bool is_valid_texture() const
//	{
//		return gl.IsTexture(texture_id);
//	}
};

struct GL32RenderTarget : public renderer::RenderTarget
{
	GLuint framebuffer;
	GLuint renderbuffer;

	GL32RenderTarget()
	{
		renderbuffer = 0;
		
		gl.GenFramebuffers(1, &framebuffer);
	}
	
	~GL32RenderTarget()
	{
		gl.DeleteFramebuffers(1, &framebuffer);
		gl.DeleteRenderbuffers(1, &renderbuffer);
	}
	
	void bind(bool activate = true)
	{
		GLuint fbo = activate ? framebuffer : 0;
		gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
	}
	
	void unbind()
	{
		bind(false);
	}
	
	void set_attachment(renderer::RenderTarget::AttachmentType type, uint8_t index, GL32Texture* texture)
	{
		bind();
		
		if (type == renderer::RenderTarget::COLOR)
		{
			gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+index, texture->texture_type, texture->texture_id, 0);
		}
		else
		{
			if (!texture)
			{
				assert(renderbuffer == 0);
			
				// create as render buffer
				gl.GenRenderbuffers(1, &renderbuffer);
				
				gl.BindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
				gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, this->width, this->height);
				gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
			}
			else
			{
				// not implemented!
				assert(0);
			}
		}
		
		unbind();
	}
	
	bool is_complete()
	{
		GLenum status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		LOGV("fbo complete? %i\n", status == GL_FRAMEBUFFER_COMPLETE);
	}
};


struct GLCore32ShaderProgram : public renderer::ShaderProgram
{
	GLint object;

	GLCore32ShaderProgram() : object(0) {}
};

GLCore32::GLCore32()
{
	LOGV( "GLCore32 instanced.\n" );
	gemgl_config config;
	config.type = renderer::OpenGL;
	config.major_version = 3;
	config.minor_version = 2;
	
	gemgl_startup( gl, config );
	last_shader = 0;
	
	image_to_internal_format = image_internal_format;
	
	
	default_render_target = CREATE(GL32RenderTarget);
	default_render_target->color_texture_id = 0;
	default_render_target->depth_texture_id = 0;
	default_render_target->width = 0;
	default_render_target->height = 0;
}

GLCore32::~GLCore32()
{
	LOGV( "GLCore32 shutting down.\n" );
	DESTROY(RenderTarget, default_render_target);
	
	gemgl_shutdown( gl );
}

void c_shader( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	ShaderProgram* shader_program;
	stream.read( shader_program );

	renderer.shaderprogram_activate( shader_program );
}

void p_shader( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	ShaderProgram* shader_program;
	stream.read( shader_program );
	
	renderer.shaderprogram_deactivate( shader_program );
}

void c_uniform_matrix4( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	int uniform_location;
	glm::mat4 * matrix = 0;
	uint8_t count = 0;
	stream.read( matrix );
	stream.read( uniform_location );
	stream.read( count );
	
	gl.UniformMatrix4fv( uniform_location, count, GL_FALSE, glm::value_ptr(*matrix) );
	gl.CheckError( "uniform matrix 4" );
}

void c_uniform1i( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	int uniform_location;
	int value;
	stream.read( uniform_location );
	stream.read( value );

	gl.Uniform1i( uniform_location, value );
	gl.CheckError( "uniform1i" );
}

void c_uniform3f( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	int uniform_location;
	float * value;
	stream.read( uniform_location );
	stream.read( value );

	gl.Uniform3fv( uniform_location, 1, value );
	gl.CheckError( "uniform3f" );
}

void c_uniform4f( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	int uniform_location;
	float * value;
	stream.read( uniform_location );
	stream.read( value );

	gl.Uniform4fv( uniform_location, 1, value );
	gl.CheckError( "uniform4f" );
}

void c_uniform_sampler2d( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	int uniform_location;
	int texture_unit;
	renderer::Texture* tex;
	GL32Texture* texture;
	
	stream.read( uniform_location );
	stream.read( texture_unit );
	stream.read( tex );
	texture = static_cast<GL32Texture*>(tex);
	if (!texture)
	{
		return;
	}
	
	//	if ( last_texture[ texture_unit ] != texture_id )
	{
		gl.ActiveTexture( GL_TEXTURE0+texture_unit );
		gl.CheckError( "ActiveTexture" );
		
		gl.BindTexture( texture->texture_type, texture->texture_id );
		gl.CheckError( "BindTexture: GL_TEXTURE_2D" );
		
		gl.Uniform1i( uniform_location, texture_unit );
		gl.CheckError( "uniform1i" );
		
//		++texture_switches;
//		last_texture[ texture_unit ] = texture_id;
	}
}

void p_uniform_sampler2d( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	int uniform_location;
	int texture_unit;
	int texture_id;
		
	stream.read( uniform_location );
	stream.read( texture_unit );
	stream.read( texture_id );
	
	gl.ActiveTexture( GL_TEXTURE0+texture_unit );
	gl.CheckError( "ActiveTexture" );
	
	gl.BindTexture( GL_TEXTURE_2D, 0 );
	gl.CheckError( "BindTexture: GL_TEXTURE_2D" );
}

void c_clear( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	unsigned int bits;
	stream.read(bits);
	gl.Clear( bits );
	gl.CheckError( "Clear" );
}

void c_clearcolor( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	float color[4];
	stream.read( color, 4*sizeof(float) );
	gl.ClearColor( color[0], color[1], color[2], color[3] );
	gl.CheckError( "ClearColor" );
}

void c_cleardepth( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	float value;
	stream.read( value );
	glClearDepth( value );
	gl.CheckError( "glClearDepth" );
}

void c_cullface( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL_LOG();
	
	renderer::CullMode cm;
	stream.read( cm );
	glCullFace( cullmode_to_gl_cullmode(cm) );
	gl.CheckError( "glCullFace" );
}

void c_viewport( util::MemoryStream & stream, GLCore32 & renderer )
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
//	GL_LOG("Viewport: %i %i %i %i", x, y, width, height);
	GL_LOG();
	gl.Viewport( x, y, width, height );
	gl.CheckError( "glViewport" );
}

void c_drawcall( util::MemoryStream & stream, GLCore32 & renderer )
{
	GL32VertexBuffer * vertex_buffer = 0;
	GLenum draw_type;
	unsigned int num_indices;
	unsigned int num_vertices;
	stream.read( vertex_buffer );
	stream.read( draw_type );
	stream.read( num_vertices );
	stream.read( num_indices );
	
	assert( vertex_buffer != 0 );
	if ( num_indices > 0 )
	{
		renderer.vertexbuffer_draw_indices( vertex_buffer, num_indices );
	}
	else
	{
		renderer.vertexbuffer_draw( vertex_buffer, num_vertices );
	}
}

void c_state( util::MemoryStream & stream, GLCore32 & renderer )
{
	// state change
	DriverState driver_state;
	
	stream.read( driver_state );
	gemgl_state_function op = operator_for_state(driver_state);
	
	op( driver_state, stream, &renderer );
}

void p_state( util::MemoryStream & stream, GLCore32 & renderer )
{
	// state change
	DriverState driver_state;
	
	stream.read( driver_state );
	gemgl_state_function op = operator_for_state(driver_state);
	
	op( driver_state, stream, &renderer );
}

void c_blendfunc( util::MemoryStream & stream, GLCore32 & renderer )
{
	RenderBlendType render_blendstate_source, render_blendstate_destination;
		
	stream.read( render_blendstate_source );
	stream.read( render_blendstate_destination );

	GLenum source = convert_blendstate( render_blendstate_source );
	GLenum destination = convert_blendstate( render_blendstate_destination );
	
	GL_LOG();
	gl.BlendFunc( source, destination );
	gl.CheckError( "BlendFunc" );
}


void c_noop( util::MemoryStream & stream, GLCore32 & renderer )
{
}

typedef void (*render_command_function)( util::MemoryStream & stream, GLCore32 & renderer );

render_command_function commands[] = {
	c_shader, // shader
	p_shader,
	
	c_uniform_matrix4, // uniform_matrix4
	c_noop,
	
	c_uniform1i, // uniform1i
	c_noop,
	
	c_uniform3f, // uniform3f
	c_noop,
	
	c_uniform4f, // uniform4f
	c_noop,
	
	c_uniform_sampler2d, // uniform_sampler_2d
	p_uniform_sampler2d,
	
	c_noop, // uniform_sampler_cube
	c_noop,

	c_clear, // clear
	c_noop,
		
	c_clearcolor, // clearcolor
	c_noop,
		
	c_cleardepth, // cleardepth
	c_noop,
	
	c_cullface, // cullface
	c_noop,
	
	c_viewport, // viewport
	c_noop,	
	
	c_drawcall, // drawcall
	c_noop,
		
	c_noop, // scissor
	c_noop,
		
	c_state, // state
	p_state,
	
	c_blendfunc, // blendfunc
	c_noop,
};


void GLCore32::init_with_settings(const RenderSettings& settings)
{
	enable_gamma_correct = settings.gamma_correct;
	if (settings.gamma_correct)
	{
		gl.Enable( GL_FRAMEBUFFER_SRGB );
		
		image_to_internal_format = ::srgb_image_to_internal_format;
	}
}

void GLCore32::run_command( renderer::DriverCommandType command, util::MemoryStream & stream )
{
	commands[ (command*2) ]( stream, *this );
}

void GLCore32::post_command( renderer::DriverCommandType command, util::MemoryStream & stream )
{
	commands[ (command*2)+1 ]( stream, *this );
}

void GLCore32::setup_drawcall( renderer::VertexBuffer * vertexbuffer, util::MemoryStream & stream )
{
	GL32VertexBuffer * vb = (GL32VertexBuffer*)vertexbuffer;
	stream.write( vb );
	stream.write( vb->gl_draw_type );
	stream.write( vb->vertex_count );
	stream.write( vb->index_count ); // or vertices
} // setup_drawcall

renderer::Texture* GLCore32::texture_create(image::Image& image)
{
	GL32Texture* texture = CREATE(GL32Texture);

	GLenum source_format = image_source_format(image.channels);
	GLenum internal_format = image_to_internal_format(image.flags);
		
	if (image.channels == 1 || image.flags & image::F_ALPHA)
	{
		texture->unpack_alignment = 1;
	}
	
	// bind the texture
	texture->bind();
	
	// setup parameters
	texture->set_parameters(image);
	
	// upload image and generate mipmaps
	gl.TexImage2D(texture->texture_type, 0, internal_format, image.width, image.height, 0, source_format, GL_UNSIGNED_BYTE, (GLvoid*)&image.pixels[0]);
	gl.GenerateMipmap(texture->texture_type);
	
	// unbind
	texture->unbind();

	return texture;
}

void GLCore32::texture_destroy(renderer::Texture* texture)
{
	GL32Texture* gltexture = static_cast<GL32Texture*>(texture);
	DESTROY(GL32Texture, gltexture);
}

void GLCore32::texture_update(renderer::Texture* texture, const image::Image& image, const gemini::Recti& rect)
{
	GL32Texture* gltexture = static_cast<GL32Texture*>(texture);
	GLenum internal_format = image_to_internal_format(image.flags);
	
	// set alignment for this operation
	if (gltexture->unpack_alignment != 4)
	{
		gl.PixelStorei(GL_UNPACK_ALIGNMENT, gltexture->unpack_alignment);
	}
	
	gltexture->bind();
	
	gl.TexSubImage2D(gltexture->texture_type, 0, rect.left, rect.top, rect.right, rect.bottom, internal_format, GL_UNSIGNED_BYTE, (GLvoid*)&image.pixels[0]);

	gltexture->unbind();
		
	// restore default alignment
	if (gltexture->unpack_alignment != 4)
	{
		gl.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
}






renderer::VertexBuffer * GLCore32::vertexbuffer_create( renderer::VertexDescriptor & descriptor, VertexBufferDrawType draw_type, VertexBufferBufferType buffer_type, unsigned int vertex_size, unsigned int max_vertices, unsigned int max_indices )
{
	GL32VertexBuffer * stream = CREATE(GL32VertexBuffer);
	assert( stream != 0 );
	
	// initial values for stream
	stream->allocate( draw_type, buffer_type );		
	
	// setup static interleaved arrays
	stream->static_setup( descriptor, vertex_size, max_vertices, max_indices );

	return stream;
} // vertexbuffer_create

void GLCore32::vertexbuffer_destroy( renderer::VertexBuffer * vertexbuffer )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	
	gl.DeleteVertexArrays( VAO_INTERLEAVED, stream->vao );
	gl.CheckError( "DeleteVertexArrays" );
		
	if ( stream->vbo[0] != 0 )
	{
		gl.DeleteBuffers( 1, stream->vbo );
		gl.CheckError( "DeleteBuffers" );
	}
	
	if ( stream->vbo[1] != 0 )
	{
		gl.DeleteBuffers( 1, &stream->vbo[1] );
		gl.CheckError( "DeleteBuffers" );
	}
	
	
	DESTROY(GL32VertexBuffer, stream);
} // vertexbuffer_destroy

void GLCore32::vertexbuffer_upload_data( VertexBuffer * vertexbuffer, unsigned int vertex_stride, unsigned int vertex_count, VertexType * vertices, unsigned int index_count, IndexType * indices )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	
	// If you hit this assert,
	// the VertexBuffer has NOT been initialized properly!
	assert( stream != 0 );
	
	gl.BindVertexArray( stream->vao[ VAO_INTERLEAVED ] );
	gl.CheckError( "BindVertexArray" );
	
	// upload interleaved array
	stream->upload_interleaved_data( vertices, vertex_count );
	
	// upload index array
	if ( indices && index_count > 0 )
	{
		stream->upload_index_array( indices, index_count );
	}
	
	gl.BindVertexArray( 0 );
	gl.CheckError( "BindVertexArray" );

	gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	gl.CheckError( "BindBuffer GL_ELEMENT_ARRAY_BUFFER" );

	gl.BindBuffer( GL_ARRAY_BUFFER, 0 );
	gl.CheckError( "BindBuffer GL_ARRAY_BUFFER" );
}


void GLCore32::vertexbuffer_draw_indices( renderer::VertexBuffer * vertexbuffer, unsigned int num_indices )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	assert( stream != 0 );
	gl.BindVertexArray( stream->vao[ VAO_INTERLEAVED ] );
	gl.CheckError( "BindVertexArray" );

	gl.DrawElements( stream->gl_draw_type, num_indices, GL_UNSIGNED_INT, 0 );
	gl.CheckError( "DrawElements" );

	gl.BindVertexArray( 0 );
	gl.CheckError( "BindVertexArray" );
}

void GLCore32::vertexbuffer_draw( renderer::VertexBuffer * vertexbuffer, unsigned int num_vertices )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	assert( stream != 0 );
	
	gl.BindVertexArray( stream->vao[ VAO_INTERLEAVED ] );
	gl.CheckError( "BindVertexArray" );

	gl.DrawArrays( stream->gl_draw_type, 0, num_vertices );
	gl.CheckError( "DrawArrays" );

	gl.BindVertexArray( 0 );
	gl.CheckError( "BindVertexArray" );
}

renderer::VertexBuffer * GLCore32::vertexbuffer_from_geometry( renderer::VertexDescriptor & descriptor, renderer::Geometry * geometry )
{
	GL32VertexBuffer * stream = CREATE(GL32VertexBuffer);
	assert( stream != 0 );
		
	renderer::VertexBufferBufferType buffer_type = renderer::BUFFER_STATIC;
	if ( geometry->is_animated() )
	{
		buffer_type = renderer::BUFFER_STREAM;
	}
	unsigned int vertex_stride = descriptor.calculate_vertex_stride();
	unsigned int max_vertices = geometry->vertex_count;
	unsigned int max_indices = geometry->index_count;
	
	stream->allocate( geometry->draw_type, buffer_type );	
	stream->static_setup( descriptor, vertex_stride, max_vertices, max_indices );

	return stream;
}

void GLCore32::vertexbuffer_upload_geometry( VertexBuffer * vertexbuffer, renderer::Geometry * geometry )
{
	GL32VertexBuffer * stream = (GL32VertexBuffer*)vertexbuffer;
	assert( stream != 0 );
	
	gl.BindVertexArray( stream->vao[ VAO_INTERLEAVED ] );
	gl.CheckError( "BindVertexArray" );
	
	unsigned int data_size = geometry->vertex_count * stream->vertex_stride;
	char * vertex_data = (char*)ALLOC( data_size );
	util::MemoryStream ms;
	ms.init( vertex_data, data_size );
	
//	assets::ShaderString parameter = "normals";
//	unsigned int normals_mask = assets::find_parameter_mask( parameter );
	
//	parameter = "colors";
//	unsigned int colors_mask = assets::find_parameter_mask( parameter );
	
//	parameter = "uv0";
//	unsigned int uv0_mask = assets::find_parameter_mask( parameter );
	
//	parameter = "uv1";
//	unsigned int uv1_mask = assets::find_parameter_mask( parameter );
	
//	parameter = "hardware_skinning";
//	unsigned int skinning_mask = assets::find_parameter_mask( parameter );
	
	for( size_t vertex_id = 0; vertex_id < geometry->vertex_count; ++vertex_id )
	{
		ms.write( &geometry->vertices[ vertex_id ], sizeof(glm::vec3) );
		
		if ( !geometry->normals.empty() )
		{
			ms.write( &geometry->normals[ vertex_id ], sizeof(glm::vec3) );
		}
		
		if ( !geometry->colors.empty() )
		{
			ms.write( &geometry->colors[ vertex_id ], sizeof(Color) );
		}

		if ( !geometry->uvs.empty() && !geometry->uvs[0].empty() )
		{
			ms.write( &geometry->uvs[0][ vertex_id ], sizeof(glm::vec2) );
		}

		if ( geometry->uvs.size() > 1 && !geometry->uvs[1].empty() )
		{
			ms.write( &geometry->uvs[1][ vertex_id ], sizeof(glm::vec2) );
		}
		
		if ( !geometry->blend_indices.empty() && !geometry->blend_weights.empty() )
		{
			ms.write( &geometry->blend_indices[ vertex_id ], sizeof(glm::vec4) );
			ms.write( &geometry->blend_weights[ vertex_id ], sizeof(glm::vec4) );
		}
	}
	
	stream->upload_interleaved_data( vertex_data, geometry->vertex_count );
	
	DEALLOC( vertex_data );
	
	if ( geometry->index_count > 0 )
	{
		stream->upload_index_array( &geometry->indices[0], geometry->index_count );
	}
	
	gl.BindVertexArray( 0 );
	gl.CheckError( "BindVertexArray" );

	gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	gl.CheckError( "BindBuffer" );

	gl.BindBuffer( GL_ARRAY_BUFFER, 0 );
	gl.CheckError( "BindBuffer" );
}

///////////////////////////////
// Shaders
// ---------------------------------------

#define SHADER_DEBUG( fmt, ... ) (void(0))
//#define SHADER_DEBUG LOGV

renderer::ShaderObject GLCore32::shaderobject_create( renderer::ShaderObjectType shader_type )
{
	GLenum type = shaderobject_type_to_gl_shaderobjecttype( shader_type );
	ShaderObject object;
	
	object.shader_id = gl.CreateShader( type );
	gl.CheckError( "CreateShader" );
	
	bool is_shader = gl.IsShader( object.shader_id );
	gl.CheckError( "IsShader: shaderobject_create" );
	if ( !is_shader )
	{
		LOGW( "object is NOT a shader\n" );
	}

	return object;
}

bool GLCore32::shaderobject_compile( renderer::ShaderObject shader_object, const char * shader_source, const char * preprocessor_defines, const char * version )
{
	bool status = true;
	const int MAX_SHADER_SOURCES = 3;
	GLint is_compiled = 0;
	const char * shaderSource[ MAX_SHADER_SOURCES ] = { version, preprocessor_defines, shader_source };
	
	// provide the sources
	gl.ShaderSource( shader_object.shader_id, MAX_SHADER_SOURCES, (GLchar**)shaderSource, 0 );
	gl.CheckError( "ShaderSource" );
	
	gl.CompileShader( shader_object.shader_id );
	gl.CheckError( "CompileShader" );

	gl.GetShaderiv( shader_object.shader_id, GL_COMPILE_STATUS, &is_compiled );
	gl.CheckError( "GetShaderiv" );

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
		status = false;
	}

	return status;
}

void GLCore32::shaderobject_destroy( renderer::ShaderObject shader_object )
{
#if 0
	bool is_shader = gl.IsShader( shader_object.shader_id );
	gl.CheckError( "IsShader: shaderobject_destroy" );
	if ( !is_shader )
	{
		LOGW( "object is NOT a shader\n" );
	}
#endif

	gl.DeleteShader( shader_object.shader_id );
	gl.CheckError( "DeleteShader" );

#if 0 // only flagged for delete when not needed by driver; (program is deleted)
	GLint delete_status = 0;
	gl.GetShaderiv( shader_object.shader_id, GL_DELETE_STATUS, &delete_status );
	gl.CheckError( "GetShaderiv shaderobject_destroy" );

	if ( !delete_status )
	{
		LOGW( "Shader not marked for delete status.\n" );
	}
#endif
}

renderer::ShaderProgram* GLCore32::shaderprogram_create()
{
	GLCore32ShaderProgram* program = CREATE(GLCore32ShaderProgram);
	program->object = gl.CreateProgram();
	gl.CheckError( "CreateProgram" );
	
	if ( !gl.IsProgram( program->object ) )
	{
		LOGE("generated object is NOT a program!\n" );
	}
	
	return program;
}

void GLCore32::shaderprogram_destroy( renderer::ShaderProgram* shader_program )
{
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	if ( program->object != 0 )
	{
		gl.DeleteProgram( program->object );
		gl.CheckError( "DeleteProgram" );
	}
}

void GLCore32::shaderprogram_attach( renderer::ShaderProgram* shader_program, renderer::ShaderObject shader_object )
{
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	gl.AttachShader( program->object, shader_object.shader_id );
	gl.CheckError( "AttachShader" );
}

void GLCore32::shaderprogram_detach( renderer::ShaderProgram* shader_program, renderer::ShaderObject shader_object )
{
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	gl.DetachShader( program->object, shader_object.shader_id );
	gl.CheckError( "DetachShader" );
}

void GLCore32::shaderprogram_bind_attributes( renderer::ShaderProgram* shader_program )
{
	// gl.BindFragDataLocation(shader_program.object, 0, parameters.frag_data_location);
	// gl.CheckError( "BindFragDataLocation" );
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	
	for(uint32_t i = 0; i < program->attributes.size(); ++i)
	{
		ShaderKeyValuePair * keyvalue = &program->attributes[i];
		SHADER_DEBUG( "BindAttribLocation -> %s to %i\n", keyvalue->first, keyvalue->second );
		gl.BindAttribLocation( program->object, keyvalue->second, keyvalue->first.c_str() );
		gl.CheckError( xstr_format( "BindAttribLocation: %s", keyvalue->first.c_str() ));
	}
}

void GLCore32::shaderprogram_bind_uniforms( renderer::ShaderProgram* shader_program )
{
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	
	// ensure this is the active shader before binding uniforms
	//this->shaderprogram_activate( shader_program );

	// fetch uniforms from the shader
	for(uint32_t uniform_id = 0; uniform_id < program->uniforms.size(); ++uniform_id)
	{
		ShaderKeyValuePair * keyvalue = &program->uniforms[ uniform_id ];
		
		keyvalue->second = gl.GetUniformLocation( program->object, keyvalue->first.c_str() );
		SHADER_DEBUG( "GetUniformLocation: \"%s\" -> %i\n", keyvalue->first.c_str(), keyvalue->second );
		gl.CheckError( "GetUniformLocation" );
		
		if ( keyvalue->second == -1 )
		{
			LOGE( "GetUniformLocation FAILED for \"%s\"\n", keyvalue->first.c_str() );
		}
	}
}

void GLCore32::shaderprogram_bind_uniform_block(renderer::ShaderProgram* shader_program, const char* block_name)
{
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	
	// find the uniform block
	GLuint block_index = gl.GetUniformBlockIndex(program->object, block_name);
	if (block_index == GL_INVALID_INDEX)
	{
		LOGV("uniform block \"%s\" could not be found\n", block_name);
		return;
	}
	LOGV("found uniform block: %s\n", block_name);
	LOGV("block_index = %u\n", block_index);
	
	// determine the size of the uniform block
	GLint block_size = 0;
	gl.GetActiveUniformBlockiv(program->object, block_index, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);
	LOGV("block_size = %i\n", block_size);
	
	// Query for the offsets of each uniform
	const char* names[] = {
		"constant_buffer.projection_matrix",
		"constant_buffer.modelview_matrix"
	};
	
	GLuint indices[] = {0, 0};
	gl.GetUniformIndices(program->object, 2, names, indices);
	for (auto i : indices)
	{
		LOGV("index: %i\n", i);
	}
	
	GLint offset[] = {-1, -1};
	gl.GetActiveUniformsiv(program->object, 2, indices, GL_UNIFORM_OFFSET, offset);
	for (auto o: offset)
	{
		LOGV("offset: %i\n", o);
	}
}

bool GLCore32::shaderprogram_link_and_validate( renderer::ShaderProgram* shader_program )
{
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	
	bool status = true;
	gl.BindFragDataLocation(program->object, 0, program->frag_data_location());
	gl.CheckError( "BindFragDataLocation" );

	gl.LinkProgram( program->object );
	gl.CheckError( "LinkProgram" );
	
	GLint link_status;
	gl.GetProgramiv( program->object, GL_LINK_STATUS, &link_status );
	gl.CheckError( "GetProgramiv" );
	
	if ( !link_status )
	{
		status = false;
		LOGE( "Error linking program!\n" );
		char * logbuffer = query_program_info_log( program->object );
		if ( logbuffer )
		{
			LOGW( "Program Info Log:\n" );
			LOGW( "%s\n", logbuffer );
			DEALLOC(logbuffer);

		}
		
//		assert( link_status == 1 );
	}
	
	// use GetAttribLocation to fetch the actual location after linking.
	// this won't work with the current render system because setting up attributes
	// doesn't use a shader.
#if 0
	else
	{
		for( unsigned int i = 0; i < parameters.total_attributes; ++i )
		{
			GLint attrib_location = gl.GetAttribLocation( shader_program.object, parameters.attributes[i].first );
			
			LOGV( "attrib: %s -> %i\n", parameters.attributes[i].first, attrib_location );
			parameters.attributes[i].second = attrib_location;
		}
	}
#endif

#if 0
	GLsizei objects = 128;
	GLuint shader_names[ 128 ] = {0};
	gl.GetAttachedShaders( shader_program.object, 128, &objects, shader_names );
	for( size_t i = 0; i < objects; ++i )
	{
		LOGV( "attached: %i\n", shader_names[i] );
	}
#endif
	
#if 0
	gl.ValidateProgram( shader_program.object );
	int validate_status;
	gl.GetProgramiv( shader_program.object, GL_VALIDATE_STATUS, &validate_status );
	gl.CheckError( "GetProgramiv" );

	if ( !validate_status )
	{
		status = false;
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

	return status;
}

void GLCore32::shaderprogram_activate( renderer::ShaderProgram* shader_program )
{
	GLCore32ShaderProgram* program = static_cast<GLCore32ShaderProgram*>(shader_program);
	
	bool is_program = gl.IsProgram( program->object );
	gl.CheckError( "IsProgram shaderprogram_activate" );
	if ( !is_program )
	{
		LOGW( "program: %i is NOT an OpenGL program\n", program->object );
		assert(is_program);
	}

	gl.UseProgram( program->object );
	gl.CheckError( "UseProgram shaderprogram_activate" );
}

void GLCore32::shaderprogram_deactivate( renderer::ShaderProgram* shader_program )
{
	gl.UseProgram( 0 );
	gl.CheckError( "UseProgram shaderprogram_deactivate" );
}

renderer::RenderTarget* GLCore32::render_target_create(uint16_t width, uint16_t height)
{
	GL32RenderTarget* rt = CREATE(GL32RenderTarget);
	
	rt->width = width;
	rt->height = height;

//	GLint is_srgb_capable = 0;
//	gl.GetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &is_srgb_capable);
//	LOGV( "srgb capable?: %s\n", (is_srgb_capable?"Yes":"No"));

	// attach the texture to the FBO
//	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, params.texture_id, 0);
//	gl.CheckError("FramebufferTexture2D");
	
	rt->color_texture_id = 0;
	rt->depth_texture_id = 0;
	// if we need a depth attachment... do that here.
//	gl.GenRenderbuffers(1, &rt->renderbuffer);
//	gl.BindRenderbuffer(GL_RENDERBUFFER, rt->renderbuffer);
//	gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
//	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->renderbuffer);
	
	
	
//	GLenum status = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
//	LOGV("fbo complete? %i\n", status==GL_FRAMEBUFFER_COMPLETE);
//	gl.CheckError("CheckFramebufferStatus");
	return rt;
}

// http://www.lighthouse3d.com/tutorials/opengl-short-tutorials/opengl_framebuffer_objects/
void GLCore32::render_target_destroy(renderer::RenderTarget* rendertarget)
{
	GL_LOG();
	GL32RenderTarget* rt = static_cast<GL32RenderTarget*>(rendertarget);
	if (!rt)
	{
		return;
	}
	gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.BindRenderbuffer(GL_RENDERBUFFER, 0);

	// delete render buffers / texture for attachments
	LOGV("TODO: purge all textures?\n");
//	gl.DeleteTextures(1, &rt->tex_params.texture_id);
	gl.DeleteFramebuffers(1, &rt->framebuffer);
	gl.DeleteRenderbuffers(1, &rt->renderbuffer);
	
	DESTROY(GL32RenderTarget, rt);
}

void GLCore32::render_target_activate(renderer::RenderTarget* rendertarget)
{
	GL_LOG();
	
	GL32RenderTarget* rt = static_cast<GL32RenderTarget*>(rendertarget);
	gl.BindFramebuffer(GL_FRAMEBUFFER, rt->framebuffer);
	GLenum drawbufs [] = { GL_COLOR_ATTACHMENT0 };
	gl.DrawBuffers(1, drawbufs);
}

void GLCore32::render_target_deactivate(renderer::RenderTarget* rendertarget)
{
	GL_LOG();
	gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLCore32::render_target_set_attachment(renderer::RenderTarget* rt, renderer::RenderTarget::AttachmentType type, uint8_t index, renderer::Texture* texture)
{
	GL32RenderTarget* render_target = static_cast<GL32RenderTarget*>(rt);
	render_target->set_attachment(type, index, static_cast<GL32Texture*>(texture));
	
	GL32Texture* tex = static_cast<GL32Texture*>(texture);
	
	if (type == renderer::RenderTarget::COLOR)
	{
		render_target->color_texture_id = tex->texture_id;
	}
	else if (type == renderer::RenderTarget::DEPTHSTENCIL)
	{
		if (tex)
		{
			render_target->depth_texture_id = tex->texture_id;
		}
	}
}

RenderTarget* GLCore32::get_default_render_target() const
{
	return default_render_target;
}

PipelineState* GLCore32::pipelinestate_create(const PipelineDescriptor& desc)
{
	return nullptr;
}

void GLCore32::pipelinestate_destroy(PipelineState* state)
{

}
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

#include "typedefs.h"
#include "memory.hpp"
#include "memorystream.hpp"
#include "image.hpp"


namespace renderer
{
	enum DriverType
	{
		Default, // pick one.
		OpenGL,
		GLESv2,
		GLESv3
	}; // DriverType
	
	
	enum DriverCommandType
	{
		DC_SHADER,
		DC_UNIFORMMATRIX4,
		DC_UNIFORM1i,
		DC_UNIFORM3f,
		DC_UNIFORM4f,
		DC_UNIFORM_SAMPLER_2D,
		DC_UNIFORM_SAMPLER_CUBE,
		
		DC_CLEAR,
		DC_CLEARCOLOR,
		DC_CLEARDEPTH,
		DC_VIEWPORT,
		
		DC_DRAWCALL,
		DC_SCISSOR,
		DC_STATE,
		DC_BLENDFUNC,
		
		DC_MAX
	}; // DriverCommandType
	
	
	enum DriverState
	{
		STATE_BLEND,
	}; // DriverState
	
	
	enum RenderBlendType
	{
		BLEND_ZERO,
		BLEND_ONE,
		BLEND_SRC_COLOR,
		BLEND_ONE_MINUS_SRC_COLOR,
		BLEND_DST_COLOR,
		BLEND_ONE_MINUS_DST_COLOR,
		BLEND_SRC_ALPHA,
		BLEND_ONE_MINUS_SRC_ALPHA,
		BLEND_ONE_MINUS_DST_ALPHA,
		BLEND_CONSTANT_COLOR,
		BLEND_ONE_MINUS_CONSTANT_COLOR,
		BLEND_CONSTANT_ALPHA,
		BLEND_ONE_MINUS_CONSTANT_ALPHA,
		BLEND_SRC_ALPHA_SATURATE,
		BLEND_SRC1_COLOR,
		BLEND_ONE_MINUS_SRC1_COLOR,
		BLEND_SRC1_ALPHA,
		BLEND_ONE_MINUS_SRC1_ALPHA		
	}; // RenderBlendType
	
	
	
	// returns 0 on failure, 1 on success
	int startup( DriverType driver );
	void shutdown();

	
	
#if PLATFORM_IS_MOBILE // assuming OpenGL ES 2.0
	typedef unsigned short IndexType;
#else
	typedef unsigned int IndexType;
#endif

	typedef unsigned char VertexType;
	
#define MAX_DESCRIPTORS 8
	typedef unsigned short VertexDescriptorType;
	
	enum
	{
		VD_FLOAT2 = 0,
		VD_FLOAT3,
		VD_FLOAT4,
		VD_UNSIGNED_BYTE3,
		VD_UNSIGNED_BYTE4,
		VD_UNSIGNED_INT,
		VD_TOTAL
	}; // Vertex Descriptor
	

	enum VertexBufferDrawType
	{
		DRAW_TRIANGLES,
		DRAW_LINES,
		DRAW_POINTS,
		
		DRAW_LIMIT,
	}; // VertexBufferDrawType
	
	enum VertexBufferBufferType
	{
		BUFFER_STATIC,
		BUFFER_DYNAMIC,
		BUFFER_STREAM,
		
		BUFFER_LIMIT,
	}; // VertexBufferBufferType
	
	enum VertexBufferErrorType
	{
		VERTEX_BUFFER_ERROR_NONE = 0,
		
	}; // VertexBufferErrorType



	enum ShaderObjectType
	{
		SHADER_VERTEX,
		SHADER_FRAGMENT,
		SHADER_GEOMETRY,
		
		SHADER_LIMIT
	}; // ShaderObjectType
	
	struct ShaderObject
	{
		unsigned int shader_id;
		short flags; // 0 on success, else error!
	}; // ShaderObject
	
	struct ShaderProgram
	{
		unsigned int object;
	}; // ShaderProgram


	
	enum ShaderErrorType
	{
		SHADER_ERROR_NONE = 0,
		SHADER_ERROR_COMPILE_FAILED,
	}; // ShaderErrorType
	
	struct VertexDescriptor
	{
		unsigned char id;
		unsigned char attribs;
		VertexDescriptorType description[ MAX_DESCRIPTORS ];
		
		static unsigned int size[ VD_TOTAL ];
		static unsigned int elements[ VD_TOTAL ];
		
		VertexDescriptor();
		void add( VertexDescriptorType desc );
		
		VertexDescriptorType get( int i );
		void reset();
	}; // VertexDescriptor
	
	
	
	
	
	
	struct TextureParameters
	{
		unsigned int image_flags;
		unsigned int channels;
		unsigned int width;
		unsigned int height;
		unsigned char * pixels;
		unsigned int texture_id;
	}; // TextureParameters
	
	
	class Font
	{
	}; // Font
	
	struct BlendParameters
	{
		unsigned int source;
		unsigned int destination;
	}; // BlendParameters
	
	
	
	class ShaderKeyValuePair : public std::pair<char*, int>
	{
	public:
	
		ShaderKeyValuePair();
		~ShaderKeyValuePair();
	
		void set_key( const char * key );
		
	};
	
//	typedef std::pair<char*, int> ShaderKeyValuePair;
	struct ShaderParameters
	{
		unsigned int total_uniforms;
		unsigned int total_attributes;
		unsigned int id;
		
		unsigned int capabilities;
		
		
		char * frag_data_location;
		
		ShaderKeyValuePair * uniforms;
		ShaderKeyValuePair * attributes;
		
		ShaderParameters();
		~ShaderParameters();
		
		void alloc_attributes( unsigned int attributes_count );
		void alloc_uniforms( unsigned int uniform_count );
		void set_frag_data_location( const char * location );

	}; // ShaderParameters
	
	struct VertexBuffer
	{
		int num_vertices;
		int num_indices;
	}; // VertexBuffer
	
	
};

#include "vertexstream.hpp"

namespace renderer
{
	//
	// IRenderDriver
	// The render driver acts as a command processor. The implementation details are up to the driver
	// which make this a nice abstraction layer.
	class IRenderDriver
	{
	public:
		virtual ~IRenderDriver() {}
		virtual const char * description() = 0;
		
		// these commands are called with the command and current memory stream
		virtual void run_command( DriverCommandType command, MemoryStream & stream ) = 0;
		virtual void post_command( DriverCommandType command, MemoryStream & stream ) = 0;

		// texture
		virtual bool upload_texture_2d( TextureParameters & parameters ) = 0;
		virtual bool generate_texture( renderer::TextureParameters & parameters ) = 0;
		virtual bool destroy_texture( renderer::TextureParameters & parameters ) = 0;
		virtual bool is_texture( renderer::TextureParameters & parameters ) = 0;
		
		// font
		virtual void render_font( int x, int y, renderer::Font & font, const char * utf8_string, const Color & color ) = 0;
		
		
		/*
		virtual void texture_activate( renderer::TextureParameters & parameters ) = 0;
		virtual void texture_deactivate( renderer::TextureParameters & parameters ) = 0;
		
		virtual void blending_activate( renderer::BlendParameters & parameters ) = 0;
		virtual void blending_deactivate( renderer::BlendParameters & parameters ) = 0;
		
		virtual void shader_activate( renderer::ShaderParameters & parameters ) = 0;
		virtual void shader_deactivate( renderer::ShaderParameters & parameters ) = 0;
		*/
		
		virtual renderer::VertexBuffer * vertexbuffer_create( renderer::VertexDescriptor & descriptor, VertexBufferDrawType draw_type, VertexBufferBufferType buffer_type, unsigned int vertex_size, unsigned int max_vertices, unsigned int max_indices ) = 0;
		virtual void vertexbuffer_destroy( renderer::VertexBuffer * stream ) = 0;
		virtual void vertexbuffer_bufferdata( VertexBuffer * vertexbuffer, unsigned int vertex_stride, unsigned int vertex_count, VertexType * vertices, unsigned int index_count, IndexType * indices ) = 0;
//		virtual void vertexbuffer_activate( renderer::VertexBuffer & parameters ) = 0;
//		virtual void vertexbuffer_update( renderer::VertexBuffer & parameters ) = 0;
		virtual void vertexbuffer_draw_indices( renderer::VertexBuffer * vertexbuffer, unsigned int num_indices ) = 0;
		virtual void vertexbuffer_draw( renderer::VertexBuffer * vertexbuffer, unsigned int num_vertices ) = 0;
//		virtual void vertexbuffer_deactivate( renderer::VertexBuffer & parameters ) = 0;
		
		
		
		virtual renderer::ShaderObject shaderobject_create( renderer::ShaderObjectType shader_type ) = 0;
		virtual bool shaderobject_compile( renderer::ShaderObject shader_object, const char * shader_source, const char * preprocessor_defines, const char * version ) = 0;
		virtual void shaderobject_destroy( renderer::ShaderObject shader_object ) = 0;
		
		virtual renderer::ShaderProgram shaderprogram_create( renderer::ShaderParameters & parameters ) = 0;
		virtual void shaderprogram_destroy( renderer::ShaderProgram program ) = 0;
		virtual void shaderprogram_attach( renderer::ShaderProgram shader_program, renderer::ShaderObject shader_object ) = 0;
		virtual void shaderprogram_bind_attributes( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters ) = 0;
		virtual void shaderprogram_bind_uniforms( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters ) = 0;
		virtual void shaderprogram_link_and_validate( renderer::ShaderProgram shader_program ) = 0;
		virtual void shaderprogram_activate( renderer::ShaderProgram shader_program ) = 0;
		virtual void shaderprogram_deactivate( renderer::ShaderProgram shader_program ) = 0;
		
	}; // IRenderDriver
	typedef IRenderDriver * (*RenderDriverCreator)();
	
	
	IRenderDriver * driver();

	
}; // namespace renderer
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
		
		DC_MAX
	}; // DriverCommandType
	
	// returns 0 on failure, 1 on success
	int startup( DriverType driver );
	void shutdown();

	
	
#if PLATFORM_IS_MOBILE // assuming OpenGL ES 2.0
	typedef unsigned short IndexType;
#else
	typedef unsigned int IndexType;
#endif
	
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
	

	enum GeometryStreamDrawType
	{
		DRAW_TRIANGLES,
		DRAW_LINES,
		DRAW_POINTS,
		
		DRAW_LIMIT,
	}; // GeometryStreamDrawType
	
	enum GeometryStreamBufferType
	{
		BUFFER_STATIC,
		BUFFER_DYNAMIC,
		BUFFER_STREAM,
		
		BUFFER_LIMIT,
	}; // GeometryStreamBufferType
	
	
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
	
	struct ShaderParameters
	{
		int object;
	}; // ShaderParameters
	
	struct GeometryStream
	{
		int num_vertices;
		int num_indices;
	}; // GeometryStream
		
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
		
		virtual renderer::GeometryStream * geometrystream_create( renderer::VertexDescriptor & descriptor, GeometryStreamDrawType draw_type, GeometryStreamBufferType buffer_type, unsigned int vertex_size, unsigned int max_vertices, unsigned int max_indices ) = 0;
		virtual void geometrystream_destroy( renderer::GeometryStream * stream ) = 0;
//		virtual void geometrystream_activate( renderer::GeometryStream & parameters ) = 0;
//		virtual void geometrystream_update( renderer::GeometryStream & parameters ) = 0;
//		virtual void geometrystream_draw_indices( unsigned int * indices, size_t num_indices ) = 0;
//		virtual void geometrystream_deactivate( renderer::GeometryStream & parameters ) = 0;
		
	}; // IRenderDriver
	typedef IRenderDriver * (*RenderDriverCreator)();
	
	
	IRenderDriver * driver();

	
}; // namespace renderer
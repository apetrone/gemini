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

#include <gemini/typedefs.h>
#include <gemini/mem.h>

#include "memorystream.h"
#include "image.h"
#include "mathlib.h" // for glm

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
		DC_CULLMODE,
		
		DC_VIEWPORT,
		
		DC_DRAWCALL,
		DC_SCISSOR,
		DC_STATE,
		DC_BLENDFUNC,
		
		DC_MAX
	}; // DriverCommandType
	
	
	enum DriverState
	{
		STATE_BACKFACE_CULLING,
		STATE_BLEND,
		STATE_DEPTH_TEST,
		STATE_DEPTH_WRITE,
	}; // DriverState
	
	enum RenderClearFlags
	{
		CLEAR_COLOR_BUFFER = 0x00004000,
		CLEAR_DEPTH_BUFFER = 0x00000100
	};
	
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
	
	enum CullMode
	{
		CULLMODE_FRONT,
		CULLMODE_BACK,
	}; // CullMode
	
	// returns 0 on failure, 1 on success
	int startup( DriverType driver );
	void shutdown();

	
	
#if PLATFORM_INDEX_TYPE == 2 // assuming OpenGL ES 2.0 (embedded or mobile platform)
	typedef unsigned short IndexType;
#elif PLATFORM_INDEX_TYPE == 1 || !defined(PLATFORM_INDEX_TYPE) // assume desktop environment
	#if PLATFORM_IS_MOBILE
		#error IndexType cannot be unsigned int on mobile!
	#endif
	typedef unsigned int IndexType;
#endif

	typedef unsigned char VertexType;
	
#define MAX_DESCRIPTORS 8
	typedef unsigned short VertexDescriptorType;
	
	// Any changes to this enum must also be handled in:
	// 1. GL Core3.2 driver (static_setup)
	// 2. VertexDescriptor::calculate_vertex_stride
	// 3. VertexDescriptor::elements (look up table for descriptor # of elements)
	// 4. VertexDescriptor::size
	enum
	{
		VD_FLOAT2 = 0,
		VD_FLOAT3,
		VD_FLOAT4,
		VD_INT4,
		VD_UNSIGNED_BYTE3,
		VD_UNSIGNED_BYTE4,
		VD_UNSIGNED_INT,
		VD_TOTAL
	}; // Vertex Descriptor
	

	enum VertexBufferDrawType
	{
		DRAW_TRIANGLES,
		DRAW_INDEXED_TRIANGLES,
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
		unsigned int calculate_vertex_stride();
		
		const VertexDescriptor & operator= ( const VertexDescriptor & other );
	}; // VertexDescriptor
	
	
	
	
	struct TextureParameters
	{
		unsigned int image_flags;
		unsigned int channels;
		unsigned int width;
		unsigned int height;
		unsigned char * pixels;
		unsigned int texture_id;
		
		unsigned int x, y;
		unsigned char alignment;
	}; // TextureParameters
	
	
	struct BlendParameters
	{
		unsigned int source;
		unsigned int destination;
	}; // BlendParameters
	
	
	struct ShaderKeyValuePair
	{
		ShaderKeyValuePair();
		virtual ~ShaderKeyValuePair();
	
		void set_key( const char * key );
		char * first;
		int second;
	};

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
		virtual ~ShaderParameters();
		
		void alloc_attributes( unsigned int attributes_count );
		void alloc_uniforms( unsigned int uniform_count );
		void set_frag_data_location( const char * location );

	}; // ShaderParameters
	
	struct VertexBuffer
	{
		int vertex_count;
		int index_count;
		
		VertexBuffer()
		{
			vertex_count = 0;
			index_count = 0;
		}
	}; // VertexBuffer
	
	struct UV
	{
		float u, v;
	};
	
	struct Geometry
	{
		unsigned int vertex_count;
		unsigned int index_count;
		renderer::VertexBufferDrawType draw_type;
		unsigned short attributes;
		bool is_animated;
				
		glm::vec3 * vertices;
		glm::vec3 * normals;
		Color * colors;
		UV * uvs;
		renderer::IndexType * indices;
		renderer::VertexBuffer * vertexbuffer;
		
		glm::ivec4* blend_indices;
		glm::vec4* blend_weights;
	}; // Geometry
	
	struct GeneralParameters
	{
		glm::mat4 * modelview_matrix;
		glm::mat4 * projection_project;
		glm::mat4 * object_matrix;
		
		unsigned int global_params;
		glm::vec3 * camera_position;
		
		
		glm::mat4* node_transforms;
		unsigned int total_node_transforms;
		
		GeneralParameters();
	};
};

#include "vertexstream.h"

namespace renderer
{
	struct VertexStream;
	
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
		
		virtual void setup_drawcall( renderer::VertexBuffer * vertexbuffer, MemoryStream & stream ) = 0;
		
		// texture
		virtual bool upload_texture_2d( TextureParameters & parameters ) = 0;
		virtual bool generate_texture( renderer::TextureParameters & parameters ) = 0;
		virtual bool destroy_texture( renderer::TextureParameters & parameters ) = 0;
		virtual bool is_texture( renderer::TextureParameters & parameters ) = 0;
		virtual bool texture_update( renderer::TextureParameters & parameters ) = 0;	
		
		virtual renderer::VertexBuffer * vertexbuffer_create( renderer::VertexDescriptor & descriptor, VertexBufferDrawType draw_type, VertexBufferBufferType buffer_type, unsigned int vertex_size, unsigned int max_vertices, unsigned int max_indices ) = 0;
		virtual void vertexbuffer_destroy( renderer::VertexBuffer * stream ) = 0;
		virtual void vertexbuffer_upload_data( VertexBuffer * vertexbuffer, unsigned int vertex_stride, unsigned int vertex_count, VertexType * vertices, unsigned int index_count, IndexType * indices ) = 0;
		
		
		virtual renderer::VertexBuffer * vertexbuffer_from_geometry( renderer::VertexDescriptor & descriptor, renderer::Geometry * geometry ) = 0;
		virtual void vertexbuffer_upload_geometry( VertexBuffer * vertexbuffer, renderer::Geometry * geometry ) = 0;
		
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
		virtual void shaderprogram_detach( renderer::ShaderProgram shader_program, renderer::ShaderObject shader_object ) = 0;
		virtual void shaderprogram_bind_attributes( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters ) = 0;
		virtual void shaderprogram_bind_uniforms( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters ) = 0;
		virtual bool shaderprogram_link_and_validate( renderer::ShaderProgram shader_program, renderer::ShaderParameters & parameters ) = 0;
		virtual void shaderprogram_activate( renderer::ShaderProgram shader_program ) = 0;
		virtual void shaderprogram_deactivate( renderer::ShaderProgram shader_program ) = 0;
	}; // IRenderDriver
	typedef IRenderDriver * (*RenderDriverCreator)();
	
	
	IRenderDriver * driver();

	
}; // namespace renderer
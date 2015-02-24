// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <core/typedefs.h>
#include <platform/mem_stl_allocator.h>

#include <core/fixedarray.h>
#include <core/stackstring.h>
#include <core/mathlib.h> // for glm
#include <core/datastream.h>

#include "image.h"

#include "shaderprogram.h"

#include "vertexbuffer.h"

#include <core/str.h>

namespace gemini
{
	namespace renderer
	{
		typedef String ShaderString;

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
		
		
		
		struct RenderSettings
		{
			// enable gamma correct rendering
			bool gamma_correct;
			
			// try to establish some sane defaults
			RenderSettings() :
				gamma_correct(false)
			{
			}
			
		};
		
		
		
		// returns 0 on failure, 1 on success
		int startup( DriverType driver, const RenderSettings& settings );
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
		

		
		struct VertexDescriptor
		{
			unsigned char id;
			unsigned char attribs;
			VertexDescriptorType description[ MAX_DESCRIPTORS ];

			static void startup();
			static void map_type(uint32_t type, uint16_t size, uint16_t elements);
			static uint16_t size[ VD_TOTAL ];
			static uint16_t elements[ VD_TOTAL ];
			
			VertexDescriptor();
			void add( VertexDescriptorType desc );
			
			VertexDescriptorType get( int i );
			void reset();
			unsigned int calculate_vertex_stride();
			
			const VertexDescriptor & operator= ( const VertexDescriptor & other );
		}; // VertexDescriptor
		
		
		enum TextureFlags
		{
			TEXTURE_WRAP 			= (1 << 0),
			TEXTURE_CLAMP 			= (1 << 1),
			TEXTURE_MIP_NEAREST		= (1 << 2),
			TEXTURE_MIP_LINEAR		= (1 << 3)
		};
		
		struct BlendParameters
		{
			unsigned int source;
			unsigned int destination;
		}; // BlendParameters
		

		
		struct Geometry
		{
			unsigned short attributes;
				
			unsigned int vertex_count;
			unsigned int index_count;
				
			// if vertex_count > 0 and these pointers are not null
			// they will be vertex_count in length.
			core::FixedArray<glm::vec3> vertices;
			core::FixedArray<glm::vec3> normals;
			core::FixedArray<core::Color> colors;
			core::FixedArray< core::FixedArray<glm::vec2> > uvs;
			core::FixedArray<glm::vec4> blend_indices;
			core::FixedArray<glm::vec4> blend_weights;
			core::FixedArray<renderer::IndexType> indices;
			
			renderer::VertexBuffer* vertexbuffer;
			renderer::VertexBufferDrawType draw_type;
			
			// return true if this object is animated
			// i.e. requires dynamic updates in the renderer
			bool is_animated() const { return 0; }
			
		}; // Geometry
	} // namespace renderer
} // namespace gemini

#include "texture.h"
#include "material.h"
#include "rendertarget.h"
#include "vertexstream.h"
#include "constantbuffer.h"
#include "pipeline.h"
#include "commandbuffer.h"

namespace gemini
{

	namespace renderer
	{
		void create_shaderprogram_from_file(const char* path, renderer::ShaderProgram** program);
	}

	struct RenderStream;

	namespace renderer
	{
		struct VertexStream;
		
		// This interface can be derived for each new platform
		class RenderDevice
		{
		public:
			virtual ~RenderDevice() {}
			
			
			
			// resource management
			
			// commands
		};
		
		
		// Renderer is a high-level interface for visuals. It doesn't know/care
		// what the underlying API or devices are.
		// Internally, it will direct calls to the device.
		class Renderer
		{
		public:
			virtual ~Renderer() {}
			
			virtual void apply_settings(const renderer::RenderSettings& settings) = 0;
		};

		//
		// IRenderDriver
		// The render driver acts as a command processor. The implementation details are up to the driver
		// which make this a nice abstraction layer.
		class IRenderDriver
		{
		public:
			virtual ~IRenderDriver() {}
			virtual const char * description() = 0;
			
			
			virtual void init_with_settings(const renderer::RenderSettings& settings) = 0;
			virtual void create_default_render_target() = 0;
			
			// these commands are called with the command and current memory stream
			virtual void run_command( DriverCommandType command, core::util::MemoryStream & stream ) = 0;
			virtual void post_command( DriverCommandType command, core::util::MemoryStream & stream ) = 0;
			
			virtual void setup_drawcall( renderer::VertexBuffer * vertexbuffer, core::util::MemoryStream & stream ) = 0;
			virtual void setup_material( renderer::Material* material, renderer::ShaderProgram* program, RenderStream& stream) = 0;
			
			// texture
			virtual renderer::Texture* texture_create(image::Image& image) = 0;
			virtual void texture_update(renderer::Texture* texture, const image::Image& image, const mathlib::Recti& rect) = 0;
			virtual void texture_destroy(renderer::Texture* texture) = 0;
			
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
			
			virtual renderer::ShaderProgram* shaderprogram_create() = 0;
			virtual void shaderprogram_destroy( renderer::ShaderProgram* program ) = 0;
			virtual void shaderprogram_attach( renderer::ShaderProgram* shader_program, renderer::ShaderObject shader_object ) = 0;
			virtual void shaderprogram_detach( renderer::ShaderProgram* shader_program, renderer::ShaderObject shader_object ) = 0;
			virtual void shaderprogram_bind_attributes( renderer::ShaderProgram* shader_program ) = 0;
			virtual void shaderprogram_bind_uniforms( renderer::ShaderProgram* shader_program ) = 0;
			virtual void shaderprogram_bind_uniform_block(renderer::ShaderProgram* shader_program, const char* block_name) = 0;
			virtual bool shaderprogram_link_and_validate( renderer::ShaderProgram* shader_program ) = 0;
			virtual void shaderprogram_activate( renderer::ShaderProgram* shader_program ) = 0;
			virtual void shaderprogram_deactivate( renderer::ShaderProgram* shader_program ) = 0;

			virtual renderer::RenderTarget* render_target_create(uint16_t width, uint16_t height) = 0;
			virtual void render_target_destroy(renderer::RenderTarget* rt) = 0;
			virtual void render_target_activate(renderer::RenderTarget* rt) = 0;
			virtual void render_target_deactivate(renderer::RenderTarget* rt) = 0;
			virtual void render_target_set_attachment(renderer::RenderTarget* rt, renderer::RenderTarget::AttachmentType type, uint8_t index, renderer::Texture* texture) = 0;
			
			
			
			virtual RenderTarget* get_default_render_target() const = 0;
			virtual PipelineState* pipelinestate_create(const PipelineDescriptor& desc) = 0;
			virtual void pipelinestate_destroy(PipelineState* state) = 0;
		}; // IRenderDriver
		typedef IRenderDriver * (*RenderDriverCreator)();
		
		
		IRenderDriver * driver();
	} // namespace renderer
} // namespace gemini
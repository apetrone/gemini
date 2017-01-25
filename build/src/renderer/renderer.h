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


#include <renderer/image.h>
#include <renderer/shader.h>
#include <renderer/vertexbuffer.h>

#include <core/typedefs.h>
#include <core/fixedarray.h>
#include <core/stackstring.h>
#include <core/mathlib.h> // for glm
#include <core/datastream.h>
#include <core/str.h>
#include <core/hashset.h>

#include <runtime/asset_handle.h>

namespace renderer
{
	const size_t GEOMETRY_UV_SET_MAX = 1;

#if defined(PLATFORM_GLES2_SUPPORT)
	typedef unsigned short IndexType;
#elif defined(PLATFORM_OPENGL_SUPPORT)
	typedef unsigned int IndexType;
#else
	#error Unknown renderer support!
#endif

	typedef unsigned char VertexType;

	// I would like to be able to test both interleaved and serial formats
	// on the GPU.
	struct GeometryVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uvs;
		//glm::vec4 blend_indices;
		//glm::vec4 blend_weights;
	};

	struct Geometry
	{
		unsigned int vertex_count;
		unsigned int index_count;

		// if vertex_count > 0 and these pointers are not null
		// they will be vertex_count in length.
		FixedArray<glm::vec3> vertices;
		FixedArray<glm::vec3> normals;
		FixedArray<glm::vec2> uvs;
		FixedArray<glm::vec4> blend_indices;
		FixedArray<glm::vec4> blend_weights;
		FixedArray<renderer::IndexType> indices;

		Geometry(gemini::Allocator& allocator);
		virtual ~Geometry();
		Geometry& operator=(const Geometry& other) = delete;

		core::StackString<128> name;

		gemini::AssetHandle material_id;
		gemini::AssetHandle shader_id;

		render2::Buffer* vertex_buffer;
		render2::Buffer* index_buffer;

		// are these still needed for any reason?
		glm::vec3 mins;
		glm::vec3 maxs;

		// model space to bone space transforms
		FixedArray<glm::mat4> bind_poses;

		// object-space to joint-space transforms
		FixedArray<glm::mat4> inverse_bind_poses;
	}; // Geometry
} // namespace renderer

#include "texture.h"
#include "material.h"
#include "rendertarget.h"
#include "constantbuffer.h"
#include "pipeline.h"
#include "commandbuffer.h"

namespace render2
{
	struct Region
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	// ---------------------------------------------------------------------
	// Texture: data uploaded to the GPU
	// ---------------------------------------------------------------------
	struct Texture
	{
		virtual ~Texture();
	};

	// This is an implementation-specific format which
	// serves as a bridge between the Shader and VertexDescriptor.
	struct InputLayout
	{
		virtual ~InputLayout();
	};
} // namespace render2

#include "constantbuffer.h"
#include "commandbuffer.h"
#include "vertexdescriptor.h"
#include "pipeline.h"

namespace render2
{
	enum class BlendOp
	{
		Zero,
		One,

		SourceAlpha,
		OneMinusSourceAlpha
	};

	enum class PrimitiveType
	{
		Lines,		// treat elements as pairs
		Triangles	// treat elements as triplets
	};

	typedef image::Image Image;

	// ---------------------------------------------------------------------
	// PipelineDescriptor
	// ---------------------------------------------------------------------
	const uint32_t MAX_PIPELINE_ATTACHMENTS = 2;
	struct PipelineDescriptor
	{
		PipelineDescriptor() :
			primitive_type(PrimitiveType::Triangles),
			blend_source(BlendOp::One),
			blend_destination(BlendOp::Zero),
			enable_blending(false)
		{
		}

		gemini::AssetHandle shader;
		uint32_t attachments[ MAX_PIPELINE_ATTACHMENTS ];
		VertexDescriptor vertex_description;
		InputLayout* input_layout;
		PrimitiveType primitive_type;

		BlendOp blend_source;
		BlendOp blend_destination;
		bool enable_blending;
	};

	// ---------------------------------------------------------------------
	// CommandSerializer
	// ---------------------------------------------------------------------
	struct CommandSerializer
	{
		virtual ~CommandSerializer() {}

		virtual void vertex_buffer(Buffer* buffer) = 0;

		virtual void draw(
			size_t initial_offset,
			size_t total,
			size_t instance_index = 0,
			size_t index_count = 1) = 0;

		virtual void draw_indexed_primitives(
			Buffer* index_buffer,
			size_t total) = 0;

		virtual void pipeline(Pipeline* pipeline) = 0;

		virtual void viewport(
			uint32_t x,
			uint32_t y,
			uint32_t width,
			uint32_t height) = 0;

		virtual void texture(
			Texture* texture,
			uint32_t index) = 0;
	}; // CommandSerializer

	// ---------------------------------------------------------------------
	// types
	// ---------------------------------------------------------------------
	typedef core::StackString<128> param_string;
	typedef HashSet<param_string, param_string> RenderParameters;
} // namespace render2

#include "device.h"

namespace render2
{
	// ---------------------------------------------------------------------
	// table of render parameters
	// <variable>: [<options/type>]
	// ---------------------------------------------------------------------
	// vsync: ["on", "off"]
	// depth_size: <uint32_t>
	// multisample: <uint32_t>
	// gamma_correct: ["true", "false"]

	// ---------------------------------------------------------------------
	//
	// ---------------------------------------------------------------------
	/// @brief Create a render device with the given parameters
	Device* create_device(gemini::Allocator& allocator, const RenderParameters& parameters);

	/// @brief Destroy an existing render device
	void destroy_device(gemini::Allocator& allocator, Device* device);
} // namespace render2


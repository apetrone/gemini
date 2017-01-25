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
#include "renderer.h"

#include "gemgl.h"
//#include "gl/opengl_common.h"

//#include <runtime/assets.h>

#include <runtime/asset_library.h>

#include <core/typedefs.h>
#include <core/logging.h>

namespace renderer
{
	Geometry::Geometry(gemini::Allocator& allocator)
		: vertices(allocator)
		, normals(allocator)
		, uvs(allocator)
		, blend_indices(allocator)
		, blend_weights(allocator)
		, indices(allocator)
		, bind_poses(allocator)
		, inverse_bind_poses(allocator)
	{
		material_id = gemini::InvalidAssetHandle;
		shader_id = gemini::InvalidAssetHandle;
		vertex_count = 0;
		index_count = 0;
		vertex_buffer = nullptr;
		index_buffer = nullptr;
	}

	Geometry::~Geometry()
	{
	}
} // namespace renderer

namespace render2
{
	Buffer::~Buffer()
	{
	}
} // namespace render2


namespace renderer
{
	int ShaderProgram::get_uniform_location( const char * name )
	{
//		LOGV( "# uniforms: %i\n", total_uniforms );
		for(size_t i = 0; i < uniforms.size(); ++i)
		{
			if (std::string(name) == uniforms[i].first)
			{
//					LOGV( "uniform: %s, at %i\n", name, uniforms[i].second );
				return uniforms[i].second;
			}
		}

		LOGW( "No uniform named %s (%i)\n", name, -1 );
		return -1;
	} // get_uniform_location

	void ShaderProgram::show_uniforms()
	{
		LOGV("uniforms:\n");
		for (size_t i = 0; i < uniforms.size(); ++i)
		{
			LOGV("%s -> %i\n", uniforms[i].first.c_str(), uniforms[i].second);
		}
	}

	void ShaderProgram::show_attributes()
	{
		LOGV("attributes:\n");
		for (size_t i = 0; i < attributes.size(); ++i)
		{
			LOGV("%s -> %i\n", attributes[i].first.c_str(), attributes[i].second);
		}
	}
}

namespace renderer
{

	// VertexTypeDescriptor
	uint16_t VertexDescriptor::size_in_bytes[ VD_TOTAL ] = {0};
	uint16_t VertexDescriptor::elements[ VD_TOTAL ] = {0};

	void VertexDescriptor::startup()
	{
		// clear table
		memset(VertexDescriptor::size_in_bytes, 0, sizeof(uint16_t)*VD_TOTAL);
		memset(VertexDescriptor::elements, 0, sizeof(uint16_t)*VD_TOTAL);

		// populate table with vertex descriptor types
		map_type(VD_FLOAT2, sizeof(float), 2);
		map_type(VD_FLOAT3, sizeof(float), 3);
		map_type(VD_FLOAT4, sizeof(float), 4);

		map_type(VD_INT4, sizeof(int), 4);

		map_type(VD_UNSIGNED_BYTE3, sizeof(unsigned char), 3);
		map_type(VD_UNSIGNED_BYTE4, sizeof(unsigned char), 4);

		map_type(VD_UNSIGNED_INT, sizeof(unsigned int), 1);

		// validate table
		for (size_t i = 0; i < VD_TOTAL; ++i)
		{
			assert(VertexDescriptor::size_in_bytes[i] != 0);
			assert(VertexDescriptor::elements[i] != 0);
		}
	}

	void VertexDescriptor::map_type(uint32_t type, uint16_t sizeof_type_bytes, uint16_t total_elements)
	{
		VertexDescriptor::size_in_bytes[type] = static_cast<uint16_t>(sizeof_type_bytes * total_elements);
		VertexDescriptor::elements[type] = total_elements;
	}

	VertexDescriptor::VertexDescriptor()
	{
		id = 0;
		attribs = 0;
		reset();
		memset( description, 0, sizeof(VertexDescriptorType) * MAX_DESCRIPTORS );
	}

	void VertexDescriptor::add( VertexDescriptorType desc )
	{
		description[ id ] = desc;
		++id;

		if ( id >= (MAX_DESCRIPTORS - 1) )
		{
			LOGV("Reached MAX_DESCRIPTORS. Resetting\n");
			id = 0;
		}

		attribs = id;
	} // add

	VertexDescriptorType VertexDescriptor::get( int i )
	{
		return description[ i ];
	} // get

	void VertexDescriptor::reset()
	{
		if ( id > 0 )
		{
			attribs = id;
		}
		id = 0;
	} // reset

	unsigned int VertexDescriptor::calculate_vertex_stride()
	{
		unsigned int stride_bytes = 0;
		for(unsigned int i = 0; i < attribs; ++i)
		{
			stride_bytes += VertexDescriptor::size_in_bytes[ description[i] ];
		}

		return stride_bytes;
	} // calculate_vertex_stride

	const VertexDescriptor & VertexDescriptor::operator= ( const VertexDescriptor & other )
	{
		this->attribs = other.attribs;
		this->id = other.id;

		for(unsigned int index = 0; index < VD_TOTAL; ++index)
		{
			this->size_in_bytes[index] = other.size_in_bytes[index];
			this->elements[index] = other.elements[index];
		}

		for(unsigned int index = 0; index < MAX_DESCRIPTORS; ++index)
		{
			this->description[index] = other.description[index];
		}

		return *this;
	} // operator=

} // namespace renderer



namespace render2
{
	// ---------------------------------------------------------------------
	// Shader
	// ---------------------------------------------------------------------
	Shader::~Shader()
	{
	}

	// ---------------------------------------------------------------------
	// Pass
	// ---------------------------------------------------------------------
	void Pass::color(float red, float green, float blue, float alpha)
	{
		target_color[0] = red;
		target_color[1] = green;
		target_color[2] = blue;
		target_color[3] = alpha;
	}

	// ---------------------------------------------------------------------
	// InputLayout
	// ---------------------------------------------------------------------
	InputLayout::~InputLayout()
	{
	}

	// ---------------------------------------------------------------------
	// Pipeline
	// ---------------------------------------------------------------------

	// ---------------------------------------------------------------------
	// Command
	// ---------------------------------------------------------------------
	Command::Command(CommandType command_type,
					 void* data0,
					 void* data1,
					 size_t param0,
					 size_t param1,
					 size_t param2,
					 size_t param3)
	{
		memset(this, 0, sizeof(Command));
		type = command_type;
		data[0] = data0;
		data[1] = data1;
		params[0] = param0;
		params[1] = param1;
		params[2] = param2;
		params[3] = param3;
	}

	// ---------------------------------------------------------------------
	// Device
	// ---------------------------------------------------------------------
	Device::~Device() {}

	// ---------------------------------------------------------------------
	// Texture
	// ---------------------------------------------------------------------
	Texture::~Texture() {}
} // namespace render2


#if defined(PLATFORM_OPENGL_SUPPORT)
	#include "gl/opengl/opengl_device.h"
#elif defined(PLATFORM_GLES2_SUPPORT)
	#include "gl/r2_gles2_device.h"
#else
	#error Unknown renderer!
#endif


namespace render2
{


	namespace detail
	{
		gemini::Allocator shader_allocator;
	} // namespace detail


	Device* create_device(gemini::Allocator& allocator, const RenderParameters& params)
	{
		// determine the renderer
		assert(params.has_key("rendering_backend"));

		detail::shader_allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_ASSETS);

		const param_string& renderer = params["rendering_backend"];
		LOGV("create device for rendering_backend '%s'\n", renderer());

		Device* device = nullptr;

#if defined(PLATFORM_OPENGL_SUPPORT)
		device = MEMORY2_NEW(allocator, OpenGLDevice)(allocator);
#elif defined(PLATFORM_GLES2_SUPPORT)
		device = MEMORY2_NEW(allocator, GLES2Device)(allocator);
#else
		#error Unknown renderer!
		return nullptr;
#endif

		// set render parameters
		device->update_parameters(params);

		return device;
	}

	void destroy_device(gemini::Allocator& allocator, Device* device)
	{
		MEMORY2_DELETE(allocator, device);
	}
} // namespace render2

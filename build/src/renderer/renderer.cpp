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
#include <renderer/renderer.h>

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
	#include "gl/opengl_device.h"
#elif defined(PLATFORM_GLES2_SUPPORT)
	#include "gl/gles2_device.h"
#else
	#error Unknown renderer!
#endif


namespace render2
{
	Device* create_device(gemini::Allocator& allocator, const RenderParameters& params)
	{
		// determine the renderer
		assert(params.has_key("rendering_backend"));

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

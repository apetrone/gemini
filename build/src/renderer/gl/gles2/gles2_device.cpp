// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "r2_gles2_device.h"

#include "glcommandserializer.h"
#include "commandbuffer.h"

namespace render2
{
	// ---------------------------------------------------------------------
	// render target
	// ---------------------------------------------------------------------
	void GLES2Device::activate_render_target(const RenderTarget& rt)
	{
	}

	void GLES2Device::deactivate_render_target(const RenderTarget& rt)
	{
	}

	RenderTarget* GLES2Device::default_render_target()
	{
		return &default_target;
	}

	RenderTarget* GLES2Device::create_render_target(Texture* texture)
	{
		return common_create_render_target(allocator, texture);
	}

	void GLES2Device::destroy_render_target(RenderTarget* target)
	{
		return common_destroy_render_target(allocator, target);
	}

	// ---------------------------------------------------------------------
	// initialization
	// ---------------------------------------------------------------------
	void GLES2Device::init(int backbuffer_width, int backbuffer_height)
	{
		default_target.width = backbuffer_width;
		default_target.height = backbuffer_height;
	}

	// ---------------------------------------------------------------------
	// command serializer
	// ---------------------------------------------------------------------
	CommandSerializer* GLES2Device::create_serializer(CommandQueue* command_queue)
	{
		static GLCommandSerializer serializer(*command_queue);
		new (&serializer) GLCommandSerializer(*command_queue);
		return &serializer;
	}

	void GLES2Device::destroy_serializer(CommandSerializer* serializer)
	{
	}

	CommandQueue* GLES2Device::create_queue(const Pass& render_pass)
	{
		return common_create_queue(render_pass, &queue.next());
	}

	// ---------------------------------------------------------------------
	// command buffers / submission
	// ---------------------------------------------------------------------
	void GLES2Device::queue_buffers(CommandQueue* queue_list, size_t total_queues)
	{
		common_queue_buffers(queue_list, total_queues, queued_buffers);
	}

	void GLES2Device::backbuffer_resized(int backbuffer_width, int backbuffer_height)
	{
		common_resize_backbuffer(backbuffer_width, backbuffer_height, &default_target);
	}

	// ---------------------------------------------------------------------
	// shader
	// ---------------------------------------------------------------------
	Shader* GLES2Device::create_shader(const char* name, Shader* reuse_shader)
	{
		return common_create_shader(allocator, "100", name, static_cast<GLShader*>(reuse_shader), "\n", "#version 100\n");
	}

	// ---------------------------------------------------------------------
	// texture
	// ---------------------------------------------------------------------
	Texture* GLES2Device::create_texture(const Image& image)
	{
		return common_create_texture(image);
	}

	void GLES2Device::update_texture(Texture* texture, const Image& image, const glm::vec2& origin, const glm::vec2& dimensions)
	{
		common_update_texture(static_cast<GLTexture*>(texture), image, origin, dimensions);
	}

	void GLES2Device::destroy_texture(Texture* texture)
	{
		common_destroy_texture(allocator, texture);
	}
} // namespace render2

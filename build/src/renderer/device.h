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
#pragma once

namespace image
{
	struct Image;
}

namespace render2
{
	enum ShaderStages
	{
		SHADER_STAGE_VERTEX,
		SHADER_STAGE_FRAGMENT
	}; // ShaderStages

	// ---------------------------------------------------------------------
	// Device: hardware abstraction layer
	// ---------------------------------------------------------------------
	// All calls to the device are executed synchronously.
	// This may or may not cause a stall on the GPU(s).

	class Device
	{
	public:
		virtual ~Device();

		// ---------------------------------------------------------------------
		// vertex / index buffers
		// ---------------------------------------------------------------------
		virtual Buffer* create_vertex_buffer(size_t size_bytes) = 0;
		virtual Buffer* create_index_buffer(size_t size_bytes) = 0;
		virtual void destroy_buffer(Buffer* buffer) = 0;

		// retrieves a pointer to buffer's data; locks it for write
		virtual void* buffer_lock(Buffer* buffer) = 0;

		// unlock a previously locked buffer
		virtual void buffer_unlock(Buffer* buffer) = 0;

		// upload data to a buffer (should not exceed buffer's max size)
		virtual void buffer_upload(Buffer* buffer, void* data, size_t data_size) = 0;

		// resize a vertex buffer in order to accomodate data_size in bytes
		virtual void buffer_resize(Buffer* buffer, size_t data_size) = 0;

		// ---------------------------------------------------------------------
		// input layout
		// ---------------------------------------------------------------------
		virtual InputLayout* create_input_layout(const VertexDescriptor& descriptor, gemini::AssetHandle shader_handle) = 0;
		virtual void destroy_input_layout(InputLayout* layout) = 0;

		// ---------------------------------------------------------------------
		// pipeline
		// ---------------------------------------------------------------------
		virtual Pipeline* create_pipeline(const PipelineDescriptor& descriptor) = 0;
		virtual void destroy_pipeline(Pipeline* pipeline) = 0;

		// ---------------------------------------------------------------------
		// shader
		// ---------------------------------------------------------------------
		virtual Shader* create_shader(ShaderSource** sources, uint32_t total_sources) = 0;
		virtual void destroy_shader(Shader* shader) = 0;


		// ---------------------------------------------------------------------
		// render target
		// ---------------------------------------------------------------------
		virtual void activate_render_target(const RenderTarget& rt) = 0;
		virtual void deactivate_render_target(const RenderTarget& rt) = 0;
		virtual RenderTarget* default_render_target() = 0;
		virtual RenderTarget* create_render_target(Texture* texture) = 0;
		virtual void destroy_render_target(RenderTarget* target) = 0;
		virtual void resize_render_target(RenderTarget* target, uint32_t width, uint32_t height) = 0;
		virtual void render_target_read_pixels(RenderTarget* target, Image& image) = 0;

		// ---------------------------------------------------------------------
		// initialization
		// ---------------------------------------------------------------------
		virtual void init(int backbuffer_width, int backbuffer_height) = 0;

		// ---------------------------------------------------------------------
		// command serializer
		// ---------------------------------------------------------------------
		virtual CommandSerializer* create_serializer(CommandQueue* command_queue) = 0;
		virtual void destroy_serializer(CommandSerializer* serializer) = 0;


		virtual CommandQueue* create_queue(const Pass& render_pass) = 0;


		// ---------------------------------------------------------------------
		// command buffers / submission
		// ---------------------------------------------------------------------
		// queue command buffers to be executed (by submit)
		virtual void queue_buffers(CommandQueue* queues, size_t total_queues) = 0;

		// submit queued command buffers to GPU
		virtual void submit() = 0;

		// handle backbuffer resize
		virtual void backbuffer_resized(int backbuffer_width, int backbuffer_height) = 0;

		// ---------------------------------------------------------------------
		// texture
		// ---------------------------------------------------------------------
		virtual Texture* create_texture(const Image& image) = 0;
		virtual void update_texture(Texture* texture, const Image& image, const glm::vec2& origin, const glm::vec2& dimensions) = 0;
		virtual void destroy_texture(Texture* texture) = 0;

		// ---------------------------------------------------------------------
		//
		// ---------------------------------------------------------------------
		virtual void update_parameters(const RenderParameters& render_params) = 0;

		virtual size_t compute_vertex_stride(const VertexDescriptor& descriptor) = 0;
		virtual size_t compute_index_stride() = 0;
	}; // class Device
} // namespace render2

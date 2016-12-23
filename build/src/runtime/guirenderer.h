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

#include <core/mathlib.h>

#include <renderer/color.h>
#include <ui/ui.h>


namespace gui
{
	class Compositor;
}

#include <renderer/renderstream.h>
#include <renderer/image.h>
#include <renderer/font.h>


// A common resource cache interface that can be used by the engine and tools/tests
// as needed.

class CommonResourceCache : public gui::ResourceCache
{
public:
	virtual void clear() {}

protected:
	virtual int track_texture(render2::Texture* /*texture*/)
	{
		return -1;
	}
//	void untrack_texture(render2::Texture* texture);

public:
	virtual gui::TextureHandle texture_to_handle(render2::Texture* /*texture*/)
	{
		return -1;
	}

	virtual render2::Texture* handle_to_texture(const gui::TextureHandle& /*handle*/)
	{
		return nullptr;
	}
};

namespace render2
{
	class Device;
	class Buffer;
	class Pipeline;
	struct Texture;
}



class GUIRenderer : public gui::Renderer
{
public:
	GUIRenderer(gemini::Allocator& guiallocator, CommonResourceCache& cache)
		: allocator(guiallocator)
		, device(nullptr)
		, vertex_buffer(nullptr)
		, gui_pipeline(nullptr)
		, font_pipeline(nullptr)
		, white_texture(nullptr)
		, resource_cache(cache)
		, vertex_cache(allocator)
	{}

	void set_device(render2::Device* render_device) { device = render_device; }

	virtual void increment_depth();

	virtual void startup(gui::Compositor* target_compositor);
	virtual void shutdown(gui::Compositor* target_compositor);

	virtual void begin_frame(gui::Compositor* target_compositor);
	virtual void end_frame();

	virtual gui::TextureResult texture_create(const char* path, gui::TextureHandle& handle);
	virtual void texture_destroy(const gui::TextureHandle& handle);
	virtual gui::TextureResult texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels);
	virtual gui::FontResult font_create(const char* path, gui::FontHandle& handle);
	virtual void font_destroy(const gui::FontHandle& handle);
	virtual gui::FontResult font_measure_string(const gui::FontHandle& handle, const char* string, size_t string_length, gui::Rect& bounds);
	virtual void font_metrics(const gui::FontHandle& handle, size_t& height, int& ascender, int& descender);
	virtual size_t font_draw(const gui::FontHandle& handle, const char* string, size_t string_length, const gui::Rect& bounds, const gemini::Color& color, gui::render::Vertex* buffer, size_t buffer_size);
	virtual size_t font_count_vertices(const gui::FontHandle& handle, size_t string_length);
//	virtual gui::TextureHandle font_get_texture(const gui::FontHandle& handle);
//	virtual gui::FontResult font_fetch_texture(const gui::FontHandle& handle, gui::TextureHandle& texture);
	virtual void draw_commands(gui::render::CommandList* command_list, Array<gui::render::Vertex>& vertex_buffer);

	GUIRenderer& operator=(const GUIRenderer& other) = delete;

private:
	gui::Compositor* compositor;
	float current_depth;

	gemini::Allocator& allocator;
	render2::Device* device;
	render2::Buffer* vertex_buffer;
	render2::Pipeline* gui_pipeline;
	render2::Pipeline* font_pipeline;
	render2::Texture* white_texture;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;
	unsigned int diffuse_texture;

	CommonResourceCache& resource_cache;
	Array<font::FontVertex> vertex_cache;
}; // GUIRenderer

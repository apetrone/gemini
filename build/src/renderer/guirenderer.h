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
#include <core/color.h>

#include <ui/ui.h>
#include <ui/compositor.h>

#include <renderer/vertexstream.h>
#include <renderer/renderstream.h>
#include <renderer/image.h>

#include <renderer/font.h>



// Aa common resource cache interface that can be used by the engine and tools/tests
// as needed.

class CommonResourceCache : public gui::ResourceCache
{
public:
	virtual void clear() {}


protected:
	virtual int track_texture(render2::Texture* texture)
	{
		return -1;
	}
//	void untrack_texture(render2::Texture* texture);

public:
	virtual gui::TextureHandle texture_to_handle(render2::Texture* texture)
	{
		return -1;
	}

	virtual render2::Texture* handle_to_texture(const gui::TextureHandle& handle)
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
	gui::Compositor* compositor;
	float current_depth;

	render2::Device* device;
	render2::Buffer* vertex_buffer;
	render2::Pipeline* gui_pipeline;
	render2::Pipeline* font_pipeline;
	render2::Texture* white_texture;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;
	unsigned int diffuse_texture;

	CommonResourceCache& resource_cache;
	Array<render2::font::FontVertex> vertices;
public:

	GUIRenderer(CommonResourceCache& cache) :
		resource_cache(cache)
	{}

	void set_device(render2::Device* device) { this->device = device; }

	virtual void increment_depth();

	virtual void startup(gui::Compositor* c);
	virtual void shutdown(gui::Compositor* c);

	virtual void begin_frame(gui::Compositor* c);
	virtual void end_frame();

	virtual void draw_bounds(const gui::Rect& bounds, const gui::Color& color);
	virtual void draw_textured_bounds(const gui::Rect& bounds, const gui::TextureHandle& handle);
	void draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color);
	virtual gui::TextureResult texture_create(const char* path, gui::TextureHandle& handle);
	virtual void texture_destroy(const gui::TextureHandle& handle);
	virtual gui::TextureResult texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels);
	virtual gui::FontResult font_create(const char* path, gui::FontHandle& handle);
	virtual void font_destroy(const gui::FontHandle& handle);
	virtual gui::FontResult font_measure_string(const gui::FontHandle& handle, const char* string, gui::Rect& bounds);
	virtual void font_metrics(const gui::FontHandle& handle, size_t& height, int& ascender, int& descender);
	virtual size_t font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color, gui::render::Vertex* buffer, size_t buffer_size);
	virtual size_t font_count_vertices(const gui::FontHandle& handle, const char* string);
	virtual gui::TextureHandle font_get_texture(const gui::FontHandle& handle);
	virtual gui::FontResult font_fetch_texture(const gui::FontHandle& handle, gui::TextureHandle& texture);
	virtual void draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists, Array<gui::render::Vertex>& vertex_buffer);
}; // GUIRenderer





#if 0
#include <assets/asset_shader.h>
#include <assets/asset_material.h>

class GUIRenderer : public gui::Renderer
{
	struct VertexType
	{
		glm::vec3 position;
		core::Color color;
		glm::vec2 uv;
	};
	
	gui::Compositor* compositor;
	::renderer::VertexStream stream;
//	::renderer::VertexStream lines;
	gemini::assets::Shader* shader;
	
//	gemini::assets::Material * solid_color;

	gemini::assets::Material * texture_map;
	
	gemini::assets::Texture* white_texture;
	
	float current_depth;
	
private:
	void render_buffer(::renderer::VertexStream& stream, gemini::assets::Shader* shader, gemini::assets::Material* material);
	
	
public:
	GUIRenderer();
	
	virtual ~GUIRenderer();
	virtual void increment_depth();
	
	virtual void startup(gui::Compositor* c);
	virtual void shutdown(gui::Compositor* c);
	
	virtual void begin_frame(gui::Compositor* c);
	virtual void end_frame();
	
	virtual void draw_bounds(const gui::Rect& bounds, const gui::Color& color);
	virtual void draw_textured_bounds(const gui::Rect& bounds, const gui::TextureHandle& handle);
	void draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color);
	virtual gui::TextureResult texture_create(const char* path, gui::TextureHandle& handle);
	virtual void texture_destroy(const gui::TextureHandle& handle);
	virtual gui::TextureResult texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels);
	virtual gui::FontResult font_create(const char* path, gui::FontHandle& handle);
	virtual void font_destroy(const gui::FontHandle& handle);
	virtual gui::FontResult font_measure_string(const gui::FontHandle& handle, const char* string, gui::Rect& bounds);
	virtual void font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gui::Color& color);
	virtual gui::FontResult font_fetch_texture(const gui::FontHandle& handle, gui::TextureHandle& texture);
	virtual void draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists);
	
}; // GUIRenderer

#endif

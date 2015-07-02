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

#include <nom/nom.hpp>
#include <nom/compositor.hpp>

#include <renderer/vertexstream.h>
#include <renderer/renderstream.h>
#include <renderer/image.h>

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
	
	unsigned int vertex_attribs;
	
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
	
	virtual void draw_bounds(const gui::Bounds& bounds, const gui::Color& color);
	virtual void draw_textured_bounds(const gui::Bounds& bounds, const gui::TextureHandle& handle);
	void draw_line(const gui::Point& start, const gui::Point& end, const gui::Color& color);
	virtual gui::TextureResult texture_create(const char* path, gui::TextureHandle& handle);
	virtual void texture_destroy(const gui::TextureHandle& handle);
	virtual gui::TextureResult texture_info(const gui::TextureHandle& handle, uint32_t& width, uint32_t& height, uint8_t& channels);
	virtual gui::FontResult font_create(const char* path, gui::FontHandle& handle);
	virtual void font_destroy(const gui::FontHandle& handle);
	virtual gui::FontResult font_measure_string(const gui::FontHandle& handle, const char* string, gui::Bounds& bounds);
	virtual void font_draw(const gui::FontHandle& handle, const char* string, const gui::Bounds& bounds, const gui::Color& color);
	virtual gui::FontResult font_fetch_texture(const gui::FontHandle& handle, gui::TextureHandle& texture);
	virtual void draw_command_lists(gui::render::CommandList** command_lists, size_t total_lists);
	
}; // GUIRenderer
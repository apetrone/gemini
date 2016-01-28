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

#include "ui/ui.h"
#include "ui/utils.h"
#include <core/array.h>
#include <renderer/color.h>

namespace gui
{
	class Compositor;

	// the rendering logic is modeled after ocornut's imgui library;
	// and also after what I've done in gemini with the
	// renderstream/vertexbuffer/vertexstream.
	namespace render
	{
		struct Vertex
		{
			float x;
			float y;
			gemini::Color color;
			float uv[2];
		};

		enum CommandType
		{
			CommandType_Generic,	// generic gui rendering command
			CommandType_Font		// font drawing command
		};

		struct Command
		{
			size_t vertex_offset;
			uint16_t vertex_count;
			uint16_t type;
			Rect clip_rect;
			TextureHandle texture;
		};

		struct CommandList
		{
			Array<Command> commands;
			Array<Vertex>* vertex_buffer;
			Compositor* compositor;

			CommandList(Compositor* compositor_instance, Array<Vertex>* buffer);
			Vertex* write_pointer;

			LIBRARY_EXPORT void reset();
			LIBRARY_EXPORT void clear();

			LIBRARY_EXPORT void push_clip_rect(const Rect& clip_rect);
			LIBRARY_EXPORT void pop_clip_rect();

			LIBRARY_EXPORT void add_drawcall();

			// primitive functions
			LIBRARY_EXPORT void primitive_reserve(size_t count);
			LIBRARY_EXPORT void primitive_quad(const Point& p0, const Point& p1, const Point& p2, const Point& p3, const TextureHandle& texture, const gemini::Color& color);

			LIBRARY_EXPORT void add_line(const Point& start, const Point& end, const gemini::Color& color, float thickness = 1.0f);
			LIBRARY_EXPORT void add_rectangle(const Point& p0, const Point& p1, const Point& p2, const Point& p3, const TextureHandle& texture, const gemini::Color& color);
			LIBRARY_EXPORT void add_font(const FontHandle& font, const char* utf8, const Rect& bounds, const gemini::Color& color);
		};
	}







	class Renderer
	{
	public:
		LIBRARY_EXPORT virtual ~Renderer() {}

		/// This is called once a compositor 'sets' a new renderer
		/// @param compositor The compositor which set this as the renderer
		LIBRARY_EXPORT virtual void startup( Compositor * compositor ) = 0;

		/// Shutdown is called when the compositor is being destroyed
		/// @param compositor The compositor which is being destroyed
		LIBRARY_EXPORT virtual void shutdown( Compositor * compositor ) = 0;

		/// This is called just before rendering is performed
		/// @param compositor The compositor about to be render
		LIBRARY_EXPORT virtual void begin_frame( Compositor * Compositor ) = 0;

		/// end_frame is called after the compositor completed has rendering
		LIBRARY_EXPORT virtual void end_frame() = 0;



		// ---------------------------------------------------------------------
		// textures
		// ---------------------------------------------------------------------

		/// Load a texture via a path and set the texture handle
		/// @param path utf-8 encoded path to texture file
		/// @param handle Output texture handle id
		/// @return TextureResult error code
		LIBRARY_EXPORT virtual TextureResult texture_create( const char * path, TextureHandle & handle ) = 0;


		/// Release a texture
		/// @param handle TextureHandle obtained from texture_create
		LIBRARY_EXPORT virtual void texture_destroy( const TextureHandle & handle ) = 0;

		/// Fetch info for a texture handle
		/// @param handle A valid texture handle retrieved from texture_create
		/// @param width The image width in pixels
		/// @param height The image height in pixels
		/// @param channels The channels (or bytes per pixel). Typically 3 or 4 (with alpha)
		/// @return TextureResult error code
		LIBRARY_EXPORT virtual TextureResult texture_info( const TextureHandle & handle, uint32_t & width, uint32_t & height, uint8_t & channels ) = 0;

		// ---------------------------------------------------------------------
		// fonts
		// ---------------------------------------------------------------------

		/// Calculate bounds for a string
		/// @param handle FontHandle to use for this operation
		/// @param string utf-8 encoded string to measure
		/// @param bounds Output bounds of string
		LIBRARY_EXPORT virtual FontResult font_measure_string(const FontHandle& handle, const char* string, gui::Rect& bounds) = 0;

		LIBRARY_EXPORT virtual void font_metrics(const gui::FontHandle& handle, size_t& height, int& ascender, int& descender) = 0;

		// ---------------------------------------------------------------------
		// command list drawing
		// ---------------------------------------------------------------------
		LIBRARY_EXPORT virtual void draw_commands(render::CommandList* command_list, Array<gui::render::Vertex>& vertex_buffer) = 0;

		/// Render a font
		/// @param handle FontHandle to use for this operation
		/// @param bounds Bounding rectangle to draw within
		/// @param color PACK_RGBA'd color value
		LIBRARY_EXPORT virtual size_t font_draw(const gui::FontHandle& handle, const char* string, const gui::Rect& bounds, const gemini::Color& color, gui::render::Vertex* buffer, size_t buffer_size) = 0;

		/// @param string utf-8 encoded string
		/// @returns Total vertices needed to draw the string
		LIBRARY_EXPORT virtual size_t font_count_vertices(const gui::FontHandle& handle, const char* string) = 0;
	}; // Renderer
} // namespace gui

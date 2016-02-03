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
#include "ui/label.h"
#include "ui/renderer.h"
#include "ui/compositor.h"

namespace gui
{
	static const float LABEL_LEFT_MARGIN = 2;
	static const float LABEL_TOP_MARGIN = 2;

	Label::Label(Panel* parent)
		: Panel(parent)
		, font_cache_index(0)
		, cache_is_dirty(1)
	{
	}

	void Label::render(Compositor* compositor,
						Renderer* renderer,
						gui::render::CommandList& render_commands)
	{
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			background_color
		);

		if (text.empty())
			return;

		size_t height;
		int ascender, descender;
		renderer->font_metrics(font_handle, height, ascender, descender);
		float font_height = (ascender + descender);

		float FONT_HEIGHT_OFFSET = font_height;

		// try and render with a font cache
		if (cache_is_dirty)
		{
			cache_is_dirty = 0;
			font_cache.clear(false);

			size_t last_start = 0;

			glm::vec2 origin_offset = glm::vec2(LABEL_LEFT_MARGIN, LABEL_TOP_MARGIN);
			origin_offset.y += FONT_HEIGHT_OFFSET;

			const size_t character_count = text.size();
			for (size_t index = 0; index < character_count+1; ++index)
			{
				const size_t is_last_character = (index == character_count);
				if (text[index] == '\n' || is_last_character)
				{
					if ((index - last_start) > 1)
					{
						font_cache_entry cs;
						cs.start = last_start;
						cs.length = (index - last_start);
						cs.origin = origin_offset;
						last_start = index;

						Rect text_bounds;
						renderer->font_measure_string(font_handle, &text[cs.start], cs.length, text_bounds);
						FONT_HEIGHT_OFFSET = glm::max(font_height, text_bounds.height());

						font_cache.push_back(cs);
						origin_offset.x = LABEL_LEFT_MARGIN;
						origin_offset.y += FONT_HEIGHT_OFFSET + font_height;
					}

					if (is_last_character)
					{
						break;
					}
				}
			}
		}

		// draw cache items
		for (const font_cache_entry& item : font_cache)
		{
			Rect current_rect = bounds;
			current_rect.origin += item.origin;
			render_commands.add_font(font_handle, &text[item.start], item.length, current_rect, foreground_color);
		}
	}

	void Label::set_font(const char* filename, size_t pixel_size)
	{
		Compositor* compositor = get_compositor();
		font_handle = compositor->get_resource_cache()->create_font(filename, pixel_size);
	}

	void Label::set_text(const std::string& utf8_string)
	{
		text = utf8_string;
		cache_is_dirty = true;
	}
} // namespace gui

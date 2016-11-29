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
#include <renderer/color.h>

TYPESPEC_REGISTER_CLASS(gui::Label);

namespace gui
{
	static const float LABEL_LEFT_MARGIN = 2;
	static const float LABEL_TOP_MARGIN = 2;

	Label::Label(Panel* parent)
		: ScrollablePanel(parent)
		, font_cache_index(0)
		, cache_is_dirty(1)
		, font_height(0)
	{
		content_bounds.set(0, 0, 0, 0);
	}

	void Label::get_content_bounds(Rect& out_bounds) const
	{
		out_bounds = content_bounds;
	}

	void Label::update(Compositor* compositor, float delta_seconds)
	{
		ScrollablePanel::update(compositor, delta_seconds);
		update_text_cache();
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

		const int32_t upper_bound = (LABEL_TOP_MARGIN - font_height);
		const int32_t lower_bound = (LABEL_TOP_MARGIN + size.height + font_height);

		// draw cache items
		float item_offset = 0.0f;
		for (const font_cache_entry& item : font_cache)
		{
			Rect current_rect;
			current_rect.origin = item.origin - scroll_offset;
			current_rect.size = size;

			// Don't draw text above the panel. This shouldn't overlap
			// by more than a single line -- clipping will take care of it.
			if (current_rect.origin.y < upper_bound)
				continue;

			// Don't draw text below the panel. This shouldn't overlap
			// by more than a single line -- clipping will take care of it.
			if ((current_rect.origin.y+item.height) > (lower_bound + item.height))
				break;

			current_rect.origin = transform_point(get_transform(0), current_rect.origin);

			render_commands.add_font(font_handle, &text[item.start], item.length, current_rect, foreground_color);
		}

		//const bool content_larger_than_bounds = content_bounds.height() > size.height;
		//if (content_larger_than_bounds)
		//{
		//	Point start(origin.x, origin.y + content_bounds.height());
		//	Point end(origin.x + size.width, origin.y + content_bounds.height());
		//	render_commands.add_line(start, end, gemini::Color::from_rgba(0, 255, 0, 255));
		//}

		render_children(compositor, renderer, render_commands);
	}

	void Label::set_font(const char* filename, size_t pixel_size)
	{
		Compositor* compositor = get_compositor();
		font_handle = compositor->get_resource_cache()->create_font(filename, pixel_size);

		// cache font_height
		size_t height;
		int ascender, descender;
		compositor->get_renderer()->font_metrics(font_handle, height, ascender, descender);
		font_height = static_cast<int32_t>(ascender + descender);
	}

	void Label::set_text(const std::string& utf8_string)
	{
		text = utf8_string;
		cache_is_dirty = 1;
	}

	void Label::append_text(const char* message)
	{
		text.append(message);
		cache_is_dirty = 1;
		update_text_cache();
	}

	void Label::update_text_cache()
	{
		// try and render with a font cache
		if (cache_is_dirty)
		{
			Renderer* renderer = get_compositor()->get_renderer();
			size_t height;
			int ascender, descender;
			renderer->font_metrics(font_handle, height, ascender, descender);
			content_bounds.origin = origin + scroll_offset;

			cache_is_dirty = 0;
			font_cache.clear(false);

			size_t last_start = 0;

			glm::vec2 origin_offset = glm::vec2(LABEL_LEFT_MARGIN, LABEL_TOP_MARGIN);
			origin_offset.y += font_height;
			content_bounds.size.width = size.width;
			content_bounds.size.height = -font_height;

			const bool enable_word_wrap = true;

			// keep track of the last segment in text to handle word wraps.
			size_t last_word_break = 0;

			const size_t character_count = text.size();
			for (size_t index = 0; index < character_count+1; ++index)
			{
				const bool is_last_character = (index == character_count);

				Rect text_bounds;

				if (text[index] == '\n' || is_last_character)
				{
					create_cache_entry(index, last_start, origin_offset);
					if (is_last_character)
					{
						break;
					}
				}
				else if (enable_word_wrap && (text[index] == ' '))
				{
					renderer->font_measure_string(font_handle, &text[last_start], (index - last_start), text_bounds);
					float test_width = content_bounds.width();
					if (has_vertical_scrollbar())
					{
						test_width -= vertical_bar->get_size().width;
					}
					if (text_bounds.width() > test_width)
					{
						// we need to wrap text starting from last index;
						create_cache_entry(last_word_break, last_start, origin_offset);
						// reset index, last_start. Advance it past the space.
						index = (last_word_break + 1);
						last_start = (index + 1);
					}
					last_word_break = index;
				}
			}
		}
	} // update_text_cache

	void Label::create_cache_entry(size_t& index, size_t& last_start, Point& origin_offset)
	{
		Renderer* renderer = get_compositor()->get_renderer();
		if ((index - last_start) > 1)
		{
			font_cache_entry cs;
			cs.start = last_start;
			cs.length = (index - last_start);
			cs.origin = origin_offset;

			last_start = index;

			Rect text_bounds;
			renderer->font_measure_string(font_handle, &text[cs.start], cs.length, text_bounds);
			size_t best_height = glm::max(font_height, text_bounds.height());

			cs.height = font_height + best_height;
			font_cache.push_back(cs);
			origin_offset.x = LABEL_LEFT_MARGIN;
			origin_offset.y += font_height + best_height;
			content_bounds.size.height += font_height + best_height;
		}
	} // create_cache_entry
} // namespace gui

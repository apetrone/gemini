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
#include "ui/ui.h"
#include "ui/panel.h"
#include "ui/button.h"
#include "ui/compositor.h"
#include "ui/renderer.h"

#include <core/logging.h>

TYPESPEC_REGISTER_CLASS(gui::Button);

namespace gui
{
	Button::Button(Panel* parent) : Panel(parent)
	{
		pressed_color = gemini::Color::from_rgba(255, 0, 0, 255);
		state = 0;
	} // Button

	Button::~Button()
	{

	} // ~Button

	void Button::handle_event(EventArgs& args)
	{
		if (args.type == Event_CursorButtonPressed)
		{
			if (args.cursor_button == CursorButton::Left)
			{
				args.compositor->set_focus(this);
				state = 2;

				EventArgs message = args;
				message.type = Event_CursorButtonPressed;

				if (on_pressed.is_valid())
				{
					on_pressed(message);
				}

				args.handled = true;
			}
		}
		else if (args.type == Event_CursorButtonReleased)
		{
			if (args.cursor_button == CursorButton::Left)
			{
				if (hit_test_local(args.local))
				{
					EventArgs message = args;
					message.type = Event_Click;
					if (on_click.is_valid())
					{
						on_click(message);
					}

					state = 1;
				}
				else
				{
					state = 0;
				}

				args.handled = true;
			}
		}
		else if (args.type == Event_CursorExit)
		{
			state = 0;
			args.handled = true;
		}
		else if (args.type == Event_CursorEnter)
		{
			state = 1;
			args.handled = true;
		}
	} // handle_event

	void Button::update(Compositor* compositor, float delta_seconds)
	{
		Panel::update(compositor, delta_seconds);

		if (state == 0)
		{
			current_color = background_color;
		}
		else if (state == 1)
		{
			current_color = hover_color;
		}
		else if (state == 2)
		{
			current_color = pressed_color;
		}

		gui::Rect font_dims;
		compositor->get_renderer()->font_measure_string(font_handle, this->text.c_str(), text.size(), font_dims);

		// We need floor to snap to pixel boundaries; not fractional pixels;
		// which would introduce artifacts.

		// centered
		text_origin.x = glm::floor((size.width / 2.0f) - (font_dims.width()/2.0f));
		text_origin.y = glm::floor((size.height / 2.0f) - (font_dims.height()/2.0f) + glm::max((float)font_height, font_dims.height()));

		// top
		//text_origin = Point(0, glm::floor(glm::max((float)font_height, font_dims.height())));
	} // update

	void Button::render(Compositor* compositor, Renderer* /*renderer*/, gui::render::CommandList& render_commands)
	{
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			current_color
		);

		gui::Rect draw_bounds;
		draw_bounds.size = size;
		draw_bounds.origin = transform_point(local_transform, text_origin);

		if (!text.empty())
		{
			render_commands.add_font(font_handle, this->text.c_str(), text.size(), draw_bounds, foreground_color);
		}
	}

	void Button::set_font(const char* filename, size_t pixel_size)
	{
		Compositor* compositor = get_compositor();
		font_handle = compositor->get_resource_cache()->create_font(filename, pixel_size);

		int ascender, descender;
		size_t height;
		compositor->get_renderer()->font_metrics(font_handle, height, ascender, descender);
		font_height = static_cast<int32_t>(ascender + descender);
	}

	void Button::set_text(const std::string& utf8_string)
	{
		text = utf8_string;
	}
} // namespace gui

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

namespace gui
{
	Button::Button(Panel * parent) : Label(parent)
	{
		on_click = 0;
		pressed_color = gui::Color(255, 0, 0);

		state = 0;
	} // Button
	
	Button::~Button()
	{
		
	} // ~Button
	
	void Button::handle_event( EventArgs & args )
	{
		if ( args.type == Event_CursorButtonPressed )
		{
			if ( args.cursor_button == CursorButton::Left )
			{
				args.compositor->set_focus( this );
				current_color = pressed_color;
				state = 1;
			}
		}
		else if ( args.type == Event_CursorButtonReleased )
		{
			if ( args.cursor_button == CursorButton::Left )
			{
				if ( args.compositor->get_hot() == this )
				{
					current_color = hover_color;

					EventArgs message = args;
					message.type = Event_Click;
					args.compositor->queue_event(message);
					state = 0;
				}
			}
		}
		else if (args.type == Event_CursorExit)
		{
			if (args.capture != this)
			{
				current_color = background_color;
				state = 0;
			}
		}
	} // handle_event
	
	void Button::update(Compositor* compositor, const TimeState& timestate)
	{
		Label::update(compositor, timestate);
		
		// TODO: calculate this when text changes?		
		gui::Rect bounds;
		get_screen_bounds(bounds);
		

		if (state == 0)
		{
			current_color = background_color;
		}

		gui::Rect font_dims;
		compositor->get_renderer()->font_measure_string(font_handle, this->text.c_str(), font_dims);

		size_t font_height;
		int ascender, descender;
		compositor->get_renderer()->font_metrics(font_handle, font_height, ascender, descender);
		font_height = (ascender + descender);

		// We need floor to snap to pixel boundaries; not fractional pixels;
		// which would introduce artifacts.
		text_origin.x = glm::floor(bounds.origin.x + (bounds.width() / 2.0f) - (font_dims.width()/2.0f));
		text_origin.y = glm::floor(bounds.origin.y + (bounds.height() / 2.0f) - (font_dims.height()/2.0f) + glm::max((float)font_height, font_dims.height()));
	} // update
	
	void Button::render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands)
	{
		if (compositor->get_hot() == this && state == 0)
		{
			current_color = hover_color;
		}
		
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			current_color
		);

		gui::Rect draw_bounds = bounds;
		draw_bounds.origin = text_origin;
		render_commands.add_font(font_handle, this->text.c_str(), draw_bounds, foreground_color);
	}
	
} // namespace gui

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
#include "ui/slider.h"
#include "ui/renderer.h"
#include "ui/compositor.h"

namespace gui
{
	// TODO: move this to the style
	static const float kLeftMargin = 4;
	static const float kRightMargin = 4;

	Slider::Slider(Panel* parent)
		: Panel(parent)
		, current_value(kLeftMargin)
		, drag_handle(nullptr)
		, drag_handle_width(0.0f)
	{
		drag_handle = new Panel(this);
		drag_handle->set_background_color(gemini::Color::from_rgba(0, 255, 255, 255));
		drag_handle->flags &= ~Flag_CursorEnabled;
		drag_handle->set_origin(0, 0);
	}

	void Slider::handle_event(gui::EventArgs &args)
	{
		if (args.type == gui::Event_CursorDrag || args.type == gui::Event_CursorButtonPressed)
		{
			// To find the 'current_value' of the slider, we work backwards by
			// taking the input at args.local.x and computing the desired position
			// for the drag handle.

			// calculate usable width
			Point left_edge = get_left_edge();
			Point right_edge = get_right_edge();
			float usable_width = (right_edge.x - left_edge.x);

			// user clicked or dragged at input
			float input = args.local.x;

			// translate it back by half the handle width
			float slider_origin = (input - (drag_handle_width / 2.0f));

			// cache the previous value
			float old_value = current_value;

			// compute the new value as it should be in our usable_width
			current_value = glm::clamp(((slider_origin - kLeftMargin) / usable_width), 0.0f, 1.0f);

			if (old_value != current_value)
			{
				on_value_changed(current_value);
			}
		}
		else if (args.type == gui::Event_KeyButtonReleased)
		{
			// TODO: handle home/end + various other keys
		}
		else if (args.type == gui::Event_CursorMove)
		{
			Point pt = args.local - drag_handle->get_origin();
			if (drag_handle->hit_test_local(pt))
			{
				drag_handle->set_background_color(gemini::Color::from_rgba(255, 0, 0, 255));
			}
			else
			{
				drag_handle->set_background_color(gemini::Color::from_rgba(0, 255, 0, 255));
			}
		}
		else if (args.type == gui::Event_CursorExit)
		{
			drag_handle->set_background_color(gemini::Color::from_rgba(0, 255, 0, 255));
		}
	}

	void Slider::update(gui::Compositor* compositor, float delta_seconds)
	{
		const float HANDLE_WIDTH_DIMENSION = 0.05f;
		const float HANDLE_HEIGHT_DIMENSION = 0.6f;

		// calculate the width of the drag handle
		Rect screen_bounds;
		get_screen_bounds(screen_bounds);
		drag_handle_width = HANDLE_WIDTH_DIMENSION * screen_bounds.width();

		Point left_edge = get_left_edge();
		Point right_edge = get_right_edge();

		float usable_width = (right_edge.x - left_edge.x);
		float xvalue = kLeftMargin + (usable_width * current_value);

		Size handle_size = get_size();

		drag_handle->set_dimensions(HANDLE_WIDTH_DIMENSION, HANDLE_HEIGHT_DIMENSION);
		float handle_height = (handle_size.height - (HANDLE_HEIGHT_DIMENSION * handle_size.height)) / 2.0f;

		drag_handle->set_origin(xvalue, handle_height);

		Panel::update(compositor, delta_seconds);
	}

	void Slider::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			background_color);

		Point left_edge = get_left_edge();
		Point right_edge = get_right_edge();
		render_commands.add_line(left_edge, right_edge, foreground_color);

		Panel::render_children(compositor, renderer, render_commands);
	}

	void Slider::set_value(float new_value)
	{
		current_value = new_value;
		on_value_changed(current_value);
	}

	Point Slider::get_left_edge()
	{
		float vertical_center = (geometry[1].y - geometry[0].y) / 2.0f;
		Point left_edge = geometry[0];
		left_edge.x += kLeftMargin + (drag_handle_width/2.0f);
		left_edge.y += vertical_center;
		return left_edge;
	}

	Point Slider::get_right_edge()
	{
		float vertical_center = (geometry[1].y - geometry[0].y) / 2.0f;
		Point right_edge = geometry[2];
		right_edge.x -= (kRightMargin + (drag_handle_width/2.0f));
		right_edge.y -= vertical_center;
		return right_edge;
	}
} // namespace gui

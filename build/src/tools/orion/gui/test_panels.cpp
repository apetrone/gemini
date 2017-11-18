// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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

#include <gui/test_panels.h>



void NumberPanel::update(gui::Compositor* compositor, float delta_seconds)
{
	gui::Label::update(compositor, delta_seconds);
}

void NumberPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
{
	gemini::Color outline_color(1.0f, 0.0f, 0.0f);

	gui::Painter painter(this, render_commands);

	//painter.add_line(
	//	geometry[0],
	//	geometry[1],
	//	outline_color
	//);

	//painter.add_line(
	//	geometry[1],
	//	geometry[2],
	//	outline_color
	//);

	//painter.add_line(
	//	geometry[2],
	//	geometry[3],
	//	outline_color
	//);

	//painter.add_line(
	//	geometry[3],
	//	geometry[0],
	//	outline_color
	//);

	gui::Label::render(compositor, renderer, render_commands);

	gui::Rect bounds;
	renderer->font_measure_string(font_handle, text.c_str(), cursor_index, bounds);

	// draw a cursor
	//const float margin = 2.0f;
	//float horizontal = bounds.size.width + margin;
	//painter.add_line(
	//	gui::Point(horizontal, 0.0f),
	//	gui::Point(horizontal, size.height),
	//	gemini::Color(1.0f, 0.0f, 0.0f),
	//	2
	//);
}

void NumberPanel::handle_event(gui::EventArgs& args)
{
	if (args.type == gui::Event_Text)
	{
		//if (args.unicode == 8)
		//{
		//	if (cursor_index > 0)
		//	{
		//		std::string::iterator it = text.begin() + (cursor_index - 1);
		//		text.erase(it);
		//		--cursor_index;
		//	}
		//}
		//else
		//{
		//	std::string::iterator it = text.begin() + cursor_index;
		//	text.resize(cursor_index);
		//	text.insert(it, (char)args.unicode);
		//	++cursor_index;
		//}
		//cache_is_dirty = 1;
	}
	else if (args.type == gui::Event_KeyButtonReleased)
	{
	}

	if (args.type == gui::Event_CursorButtonPressed)
	{
		is_captured = 1;
		down_position = args.local;
		args.handled = 1;
		args.compositor->set_focus(this);
	}
	else if (args.type == gui::Event_CursorButtonReleased)
	{
		is_captured = 0;
		args.handled = 1;
	}

	if (args.type == gui::Event_CursorMove)
	{
		if (is_captured)
		{
			gui::Point delta = args.local - down_position;
			value += delta.x * step;
			set_text(core::str::format("%2.4f", value));
			if (value_changed.is_valid())
			{
				value_changed(value);
			}
			args.handled = 1;
		}
	}

	if (args.type == gui::Event_CursorButtonReleased)
	{
		if (args.cursor_button == gui::CursorButton::Right)
		{
			if (cursor_index < text.length())
			{
				cursor_index += 1;
				args.handled = 1;
			}
		}
		else if (args.cursor_button == gui::CursorButton::Left)
		{
			if (cursor_index > 0)
			{
				cursor_index -= 1;
				args.handled = 1;
			}
		}
	}
}
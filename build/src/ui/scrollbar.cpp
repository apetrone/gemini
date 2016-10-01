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
#include "ui/scrollbar.h"
#include "ui/renderer.h"
#include "ui/compositor.h"
#include <renderer/color.h>

#include <core/logging.h>

TYPESPEC_REGISTER_CLASS(gui::ScrollButton);
TYPESPEC_REGISTER_CLASS(gui::Scrollbar);

namespace gui
{
	ScrollButton::ScrollButton(gui::Panel* parent, uint32_t direction)
		: Button(parent)
		, scroll_direction(direction)
		, scroll_value(0.0f)
	{
	}

	Scrollbar::Scrollbar(gui::Panel* parent, uint32_t direction)
		: gui::Panel(parent)
	{
		set_background_color(gemini::Color::from_rgba(64, 64, 64, 255));

		bootun = new ScrollButton(this, direction);
		bootun->set_origin(0, 0);
		bootun->set_font("fonts/debug.ttf", 16);
		bootun->set_background_color(gemini::Color::from_rgba(120, 120, 120, 255));
		bootun->set_hover_color(gemini::Color(0, 0, 0));
		bootun->set_flags(0);
		bootun->set_name("scrollbar_bootun");
		bootun->set_visible(false);

		deferred_flags = 0;
		set_flags(get_flags() | Flag_CanMove);

		is_dragging = false;
	}

	void Scrollbar::handle_event(gui::EventArgs& args)
	{
		if (args.target)
			LOGV("handle_event for %s\n", args.target->get_name());

		if (args.type == gui::Event_CursorDrag)
		{
			if (is_dragging)
			{
				LOGV("move scrollbar\n");
				const float max_y = (size.height - bootun->get_size().height);
				const float new_y = glm::clamp(args.local.y - initial_click.y, 0.0f, max_y);
				set_scroll_value((new_y / max_y));
				args.handled = true;
				args.compositor->set_focus(bootun);
			}
		}
		else if (args.type == gui::Event_CursorButtonPressed)
		{
			Point pos = bootun->compositor_to_local(args.cursor);
			if (bootun->hit_test_local(pos))
			{
				initial_click = pos;
				is_dragging = true;
				bootun->set_background_color(gemini::Color::from_rgba(128, 64, 0, 255));
				args.handled = true;
			}
			else
			{
				LOGV("unhandled thing\n");
			}
		}
		else if (args.type == gui::Event_CursorButtonReleased)
		{
			is_dragging = false;
			bootun->set_background_color(gemini::Color::from_rgba(255, 128, 0, 255));
			args.handled = true;
		}
		if (args.type == Event_CursorMove || args.type == Event_CursorExit)
		{
			const Point local_point = bootun->compositor_to_local(args.cursor);
			if (bootun->hit_test_local(local_point))
			{
				bootun->set_background_color(gemini::Color::from_rgba(255, 128, 0, 255));
			}
			else
			{
				bootun->set_background_color(gemini::Color::from_rgba(120, 120, 120, 255));
			}

			args.handled = true;
		}

		Panel::handle_event(args);
	}

	void Scrollbar::set_button_size(float width, float height)
	{
		bootun->set_visible(true);
		bootun->set_size(width, height);
	}

	void Scrollbar::set_scroll_value(float new_value)
	{
		new_value = glm::clamp(new_value, 0.0f, 1.0f);
		const float max_y = (size.height - bootun->get_size().height);
		const float vertical_position = new_value * max_y;
		bootun->set_origin(0, vertical_position);
		if (on_scroll_value_changed.is_valid())
		{
			on_scroll_value_changed(new_value);
		}
		flags |= Flag_TransformIsDirty;
	}
} // namespace gui

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
#include "ui/scrollable.h"
#include "ui/renderer.h"
#include "ui/compositor.h"
//#include <renderer/color.h>

namespace gui
{
	static const float SCROLL_BAR_WIDTH = 12;

	ScrollablePanel::ScrollablePanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
		// horizontal_bar = new Scrollbar(this, 0);
		// horizontal_bar->on_scroll_value_changed.connect(&ScrollablePanel::on_horizontal_scroll, this);
		// horizontal_bar->set_visible(false);
		// horizontal_bar->set_name("horizontal_scrollbar");

		vertical_bar = new Scrollbar(this, 1);
		vertical_bar->on_scroll_value_changed.connect(&ScrollablePanel::on_vertical_scroll, this);
		vertical_bar->set_visible(false);
		vertical_bar->set_name("vertical_scrollbar");
	}

	void ScrollablePanel::update(gui::Compositor* compositor, float delta_seconds)
	{
		Panel::update(compositor, delta_seconds);
	} // update

	void ScrollablePanel::handle_event(gui::EventArgs& args)
	{
		Panel::handle_event(args);

		if (args.type == Event_CursorScroll)
		{
			// TODO: Handle cursor scrolling. Should update the content
			// scroll and scroll bar position.
//			if (vertical_bar->is_visible())
//			{
//				gui::Rect content_rect;
//				get_content_bounds(content_rect);
//				scroll_offset.y -= (args.wheel* 5.0f);
//				if (scroll_offset.y < 0.0f)
//					scroll_offset.y = 0;
//			}
		}
		else if (args.type == Event_KeyButtonPressed)
		{
			// TODO: handle home, end, page up, page down, left arrow, right arrow
		}
	} // handle_event


	void ScrollablePanel::scroll_to_bottom()
	{
		// ensure scroll bar sizes have been updated
		// before we attempt to modify the scroll value.
		update_scrollbars();

		vertical_bar->set_scroll_value(1.0f);
	}

	void ScrollablePanel::update_scrollbars()
	{
		gui::Rect content_rect;
		get_content_bounds(content_rect);

		// get the vertical ratio
		const float vertical_content_overflow = (content_rect.height() - size.height);
		if (vertical_content_overflow > 0)
		{
			float vratio = size.height / content_rect.height();
			vratio = glm::min(vratio, 1.0f);
			vertical_bar->set_origin(size.width-SCROLL_BAR_WIDTH, 0);
			vertical_bar->set_size(gui::Size(SCROLL_BAR_WIDTH, size.height-SCROLL_BAR_WIDTH));
			vertical_bar->set_button_dimensions(1.0f, vratio);
			vertical_bar->set_visible(vertical_content_overflow ? true : false);
		}
	}

	void ScrollablePanel::on_vertical_scroll(float value)
	{
		gui::Rect content_rect;
		get_content_bounds(content_rect);

		const float vertical_content_overflow = (content_rect.size.height - size.height);
		if (vertical_content_overflow > 0)
		{
			scroll_offset.y = (value * vertical_content_overflow);
		}
	} // on_vertical_scroll

	void ScrollablePanel::on_horizontal_scroll(float value)
	{
		LOGV("horizontal: %2.2f\n", value);
	} // on_horizontal_scroll
} // namespace gui

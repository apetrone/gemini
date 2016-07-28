// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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
#include "ui/dockingcontainer.h"
#include "ui/renderer.h"
#include "ui/compositor.h"
#include <renderer/color.h>

#include <core/logging.h>

namespace gui
{
	DockingContainer::DockingContainer(Panel* parent)
		: Panel(parent)
	{
		for (size_t index = 0; index < 5; ++index)
		{
			colors[index] = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
		}

		regions[0].set(0.0f, 0.0f, 50.0f, 50.0f);

		//flags |= Flag_CanMove;
		flags |= Flag_CanDrop;
	}

	void DockingContainer::handle_event(EventArgs& args)
	{
		if (args.type == Event_CursorMove)
		{
			if (regions[0].is_point_inside(args.local))
			{
				//if (args.compositor->get_capture())
				{
					colors[0] = gemini::Color(1.0f, 0.0f, 0.0f);
				}
			}
			else
			{
				colors[0] = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
			}

			args.handled = 1;
		}
		else if (args.type == Event_CursorDragEnter)
		{
			LOGV("DockingContainer handle Event_CursorDragEnter\n");
			colors[0] = gemini::Color(1.0f, 1.0f, 0.0f);
		}
		else if (args.type == Event_CursorDragExit)
		{
			LOGV("DockingContainer handle Event_CursorDragExit\n");
			colors[0] = gemini::Color(0.0f, 0.0f, 0.0f);
		}
		else if (args.type == Event_CursorDrop)
		{
			LOGV("docking container handle Event_CursorDrop at %2.2f, %2.2f\n", args.local.x, args.local.y);
			colors[0] = gemini::Color(0.0f, 0.0f, 1.0f);
		}

		Panel::handle_event(args);
	}

	void DockingContainer::update(Compositor* compositor, float delta_seconds)
	{
		//for (size_t index = 0; index < 5; ++index)
		//{
		//	colors[index] = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
		//}



		Panel::update(compositor, delta_seconds);
	}

	void DockingContainer::render(Compositor* compositor,
						Renderer* renderer,
						gui::render::CommandList& render_commands)
	{
		Panel::render(compositor, renderer, render_commands);


		gui::Point region[4];



		region[0] = transform_point(get_transform(0), Point(regions[0].origin.x, regions[0].origin.y+regions[0].size.height));
		region[1] = transform_point(get_transform(0), Point(regions[0].origin.x+regions[0].size.width, regions[0].origin.y+regions[0].size.height));
		region[2] = transform_point(get_transform(0), Point(regions[0].origin.x+regions[0].size.width, regions[0].origin.y));
		region[3] = transform_point(get_transform(0), Point(regions[0].origin.x, regions[0].origin.y));

		render_commands.add_rectangle(
			region[0],
			region[1],
			region[2],
			region[3],
			render::WhiteTexture,
			colors[0]
		);

#if 0
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
#endif
	}

} // namespace gui

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

TYPESPEC_REGISTER_CLASS(gui::DockingContainer);

namespace gui
{
	DockingContainer::DockingContainer(Panel* parent)
		: Panel(parent)
	{
		reset_colors();
		flags |= Flag_CanDrop;
	} // DockingContainer

	void DockingContainer::reset_colors()
	{
		for (size_t index = 0; index < 5; ++index)
		{
			colors[index] = gemini::Color(0.0f, 0.0f, 0.0f, 0.0f);
		}
	} // reset_colors

	void DockingContainer::handle_event(EventArgs& args)
	{
		if (args.type == Event_CursorMove)
		{
			reset_colors();

			for (size_t index = 0; index < 5; ++index)
			{
				if (regions[index].is_point_inside(args.local))
				{
					if (args.compositor->get_capture())
					{
						colors[index] = gemini::Color(0.0f, 0.25f, 0.5f, 0.5f);
						break;
					}
				}
			}

			args.handled = 1;
		}
		else if (args.type == Event_CursorDragEnter)
		{
		}
		else if (args.type == Event_CursorDragExit)
		{
			reset_colors();
		}
		else if (args.type == Event_CursorDropMove)
		{
			reset_colors();
			for (size_t index = 0; index < 5; ++index)
			{
				if (regions[index].is_point_inside(args.local))
				{
					colors[index] = gemini::Color(0.0f, 0.25f, 0.5f, 0.5f);
					break;
				}
			}

			args.handled = 1;
		}
		else if (args.type == Event_CursorDrop)
		{
			for (size_t index = 0; index < 5; ++index)
			{
				if (regions[index].is_point_inside(args.local))
				{
					LOGV("docking container handle Event_CursorDrop at %2.2f, %2.2f\n", args.local.x, args.local.y);

					add_child(args.capture);
					args.capture->set_size(Size(regions[index].width(), regions[index].height()));
					args.capture->set_origin(regions[index].origin);
					args.handled = 1;
					break;
				}
			}

			reset_colors();
		}

		Panel::handle_event(args);
	} // handle_event

	void DockingContainer::update(Compositor* compositor, float delta_seconds)
	{
#if 0
		Panel::update(compositor, delta_seconds);

		float width = dimensions.x;
		float height = dimensions.y;

		float side_width = (width * 0.15f);
		float rect_width = 2 * side_width;
		float center_width = width - rect_width;

		float vert_height = (height * 0.15f);
		float rect_height = 2 * vert_height;
		float center_height = height - rect_height;

		Point leftover = pixels_from_dimensions(Point(center_width+side_width, center_height + vert_height));

		Point topleft = pixels_from_dimensions(Point(side_width, 0.0f));

		Point pix = pixels_from_dimensions(Point(side_width, vert_height));

		Point full = pixels_from_dimensions(dimensions);
		Point cw = pixels_from_dimensions(Point(center_width, center_height));

		// left
		regions[0].set(0.0f, 0.0f, pix.x, full.y);

		// top
		regions[1].set(topleft.x, 0.0f, cw.x, pix.y);

		// right
		regions[2].set(leftover.x, 0.0f, pix.x, full.y);

		// bottom
		regions[3].set(topleft.x, leftover.y, cw.x, pix.y);

		// center
		regions[4].set(pix.x, pix.y, cw.x, cw.y);
#endif
	} // update

	void DockingContainer::render(Compositor* compositor,
						Renderer* renderer,
						gui::render::CommandList& render_commands)
	{
		Panel::render(compositor, renderer, render_commands);

		// draw all regions
		for (size_t index = 0; index < 5; ++index)
		{
			gui::Point region[4];
			region[0] = transform_point(get_transform(0), Point(regions[index].origin.x, regions[index].origin.y + regions[index].size.height));
			region[1] = transform_point(get_transform(0), Point(regions[index].origin.x + regions[index].size.width, regions[index].origin.y + regions[index].size.height));
			region[2] = transform_point(get_transform(0), Point(regions[index].origin.x + regions[index].size.width, regions[index].origin.y));
			region[3] = transform_point(get_transform(0), Point(regions[index].origin.x, regions[index].origin.y));

			render_commands.add_rectangle(
				region[0],
				region[1],
				region[2],
				region[3],
				render::WhiteTexture,
				colors[index]
			);
		}
	} // render

} // namespace gui

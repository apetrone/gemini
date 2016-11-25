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
#include "ui/timeline.h"
#include "ui/renderer.h"
#include "ui/compositor.h"
#include <renderer/color.h>

#include <core/logging.h>

TYPESPEC_REGISTER_CLASS(gui::TimelineScrubber);
TYPESPEC_REGISTER_CLASS(gui::Timeline);

namespace gui
{
	TimelineScrubber::TimelineScrubber(Panel* parent)
		: Panel(parent)
	{
		flags |= Flag_CursorEnabled;
		set_name("TimelineScrubber");

		flags |= Flag_CanMove;
	}

	void TimelineScrubber::render(gui::Compositor* /*compositor*/, gui::Renderer* /*renderer*/, gui::render::CommandList& render_commands)
	{
		// TODO: we should get this from the style
		gemini::Color scrubber_highlight = gemini::Color::from_rgba(255, 128, 0, 32);
		gemini::Color scrubber_outline = gemini::Color::from_rgba(255, 128, 0, 192);

		// draw the main highlight fill
		render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, scrubber_highlight);

		// draw the outline
		render_commands.add_line(geometry[0], geometry[1], scrubber_outline);
		render_commands.add_line(geometry[1], geometry[2], scrubber_outline);
		render_commands.add_line(geometry[2], geometry[3], scrubber_outline);
		render_commands.add_line(geometry[3], geometry[0], scrubber_outline);
	}

	bool TimelineScrubber::point_in_capture_rect(const Point& local) const
	{
		return hit_test_local(local);
	}


	Timeline::Timeline(Panel* parent)
		: Panel(parent)
		, left_margin(0)
		, current_frame(0)
		, frame_width_pixels(0.0f)
	{
		flags |= Flag_CursorEnabled;
		set_name("Timeline");

		scrubber = new TimelineScrubber(this);
		scrubber->set_origin(0.0f, 0.0f);
	}


	void Timeline::handle_event(EventArgs& args)
	{
		last_position = args.local;
		if (args.type == Event_CursorDrag || args.type == Event_CursorButtonPressed)
		{
			if (args.type == Event_CursorButtonPressed)
			{
				args.compositor->set_capture(scrubber, args.cursor_button);
			}
			// snap to the closest point
			size_t last_frame = current_frame;

			int next_frame = (((int)args.local.x) - 1) / frame_width_pixels;

			if (next_frame < 0)
				current_frame = 0;
			else if (next_frame > (int)total_frames)
				current_frame = total_frames;
			else
				current_frame = next_frame;

			if (current_frame <= 0)
			{
				current_frame = 0;
			}
			else if (current_frame > total_frames - 1)
			{
				current_frame = total_frames - 1;
			}

			if ((last_frame != current_frame) && on_scrubber_changed.is_valid())
			{
				on_scrubber_changed(current_frame);
			}

			args.handled = true;
		}
	} // handle_event

	void Timeline::update(gui::Compositor* compositor, float delta_seconds)
	{
		// assuming a horizontal timeline

		// recompute the distance here
		frame_width_pixels = (size.width / (float)total_frames);

		// should be updated before rendering
		assert(frame_width_pixels > 0);

		scrubber->set_size(frame_width_pixels, size.height);
		scrubber->set_origin((current_frame * frame_width_pixels), 0.0f);

		Panel::update(compositor, delta_seconds);
	} // update

	void Timeline::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		// TODO: should get this from the style
		const gemini::Color frame_color = gemini::Color::from_rgba(96, 96, 96, 255);

		// draw the background
		render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, gemini::Color::from_rgba(64, 64, 64, 255));

		// add a top rule line to separate this panel
		render_commands.add_line(geometry[0], geometry[3], gemini::Color::from_rgba(0, 0, 0, 255), 1.0f);

		// center the individual frames
		Rect block;
		block.set(left_margin + 2.0f, 1.0, (frame_width_pixels - 4.0f), size.height - 2.0f);

		for (size_t index = 0; index < total_frames; ++index)
		{
			// draw frame ticks until we reach the end of the panel
			if (block.origin.x + block.size.width >= (origin.x + size.width))
			{
				break;
			}

			Point points[4];
			points[0] = transform_point(local_transform, block.origin);
			points[1] = transform_point(local_transform, Point(block.origin.x, block.origin.y + block.size.height));
			points[2] = transform_point(local_transform, Point(block.origin.x + block.size.width, block.origin.y + block.size.height));
			points[3] = transform_point(local_transform, Point(block.origin.x + block.size.width, block.origin.y));
			render_commands.add_rectangle(points[0], points[1], points[2], points[3], gui::render::WhiteTexture, frame_color);

			block.origin.x += frame_width_pixels;
		}

		render_children(compositor, renderer, render_commands);
	} // render

	void Timeline::set_frame_range(int lower_frame_limit, int upper_frame_limit)
	{
		lower_limit = lower_frame_limit;
		upper_limit = upper_frame_limit;

		assert(upper_limit > lower_limit);
		total_frames = (upper_limit - lower_limit);

		// force a recalculate on the next render call
		frame_width_pixels = 0;
	} // set_frame_range

	void Timeline::set_frame(size_t frame)
	{
		current_frame = frame;
	} // set_frame

	bool Timeline::point_in_capture_rect(const Point&) const
	{
		return true;
	} // point_in_capture_rect
} // namespace gui

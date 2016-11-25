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
#pragma once

#include "ui/panel.h"
#include "ui/events.h"

#include <core/typedefs.h>
#include <core/typespec.h>

namespace gui
{
	class TimelineScrubber : public Panel
	{
		TYPESPEC_DECLARE_CLASS(TimelineScrubber, Panel);

	public:
		TimelineScrubber(Panel* parent);
		virtual void render(gui::Compositor* /*compositor*/, gui::Renderer* /*renderer*/, gui::render::CommandList& render_commands);
		virtual bool point_in_capture_rect(const Point& local) const override;
	}; // TimelineScrubber

	class Timeline : public Panel
	{
		TYPESPEC_DECLARE_CLASS(Timeline, Panel);

	public:
		Timeline(Panel* parent);

		gemini::Delegate<void(size_t)> on_scrubber_changed;

		virtual void handle_event(EventArgs& args) override;

		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
		void set_frame_range(int lower_frame_limit, int upper_frame_limit);
		void set_frame(size_t frame);
		virtual bool point_in_capture_rect(const Point&) const override;

	private:
		size_t left_margin;
		size_t current_frame;
		size_t total_frames;

		// frame limits
		int lower_limit;
		int upper_limit;

		// width of a clickable 'frame'
		float frame_width_pixels;

		Point last_position;

		TimelineScrubber* scrubber;
	}; // Timeline
} // namespace gui

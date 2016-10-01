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
#pragma once

#include "ui/panel.h"
#include "ui/utils.h"

#include <core/typespec.h>

#include <stdint.h>

namespace gui
{
	// This graph will scroll at the rate that values are recorded;
	// not necessarily by time.

	class Graph : public Panel
	{
		TYPESPEC_DECLARE_CLASS(Graph, Panel);
	protected:
		float* values;
		bool show_baseline;

		float range_min;
		float range_max;

		Point* last_point;
		gemini::Color* channel_colors;

		// 0: left, 1: right
		Point baseline_start;
		Point baseline_end;
		gemini::Color baseline_color;
		Point baseline_text_origin;

		float baseline_value;

		uint32_t total_samples;
		uint32_t total_channels;
		uint32_t* current_sample;

		FontHandle font_handle;
		gemini::Color background_color;
		gemini::Color foreground_color;

		// vertices for the lines (total_lines * 2)
		Point* vertices;
		gemini::Color* vertex_colors;

		// 0: top, 1: bottom
		Point range_text_origin[2];

		float font_height;

		enum ChannelType
		{
			ChannelColor,
			ChannelMin,
			ChannelMax,
			ChannelTotal
		};

	private:
		void draw_float(Renderer* renderer, float value, const Point& pt, const gemini::Color& color, gui::render::CommandList& render_commands);

	public:
		Graph(Panel* parent);
		virtual ~Graph();

		void create_samples(uint32_t max_samples, uint32_t max_channels);
		void configure_channel(uint32_t channel_index, const gemini::Color& color/*, const gemini::Color& min, const gemini::Color& max*/);
		void enable_baseline(bool enabled, float value = 0.0f, const gemini::Color& color = gemini::Color());
		bool channel_in_range(uint32_t channel_index) const;
		void record_value(float value, uint32_t channel_index);
		void set_range(float min_range, float max_range);

		void set_font(const char* filename, size_t pixel_size);
		virtual void set_background_color(const gemini::Color& color);
		virtual void set_foreground_color(const gemini::Color& color);

		// Panel overrides
		virtual void render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands) override;
		virtual void update(Compositor* compositor, float delta_seconds) override;
	}; // Graph
} // namespace gui

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

#include <stdint.h>


namespace gui
{
	// This graph will scroll at the rate that values are recorded;
	// not necessarily by time.

	class Graph : public Panel
	{
	protected:
		float* values;
		bool show_baseline;
		
		int32_t range_min;
		int32_t range_max;
		
		Point* last_point;
		gui::Color* channel_colors;
		gui::Color baseline_color;
		float baseline_value;
		
		uint32_t flags;
		uint32_t total_samples;
		uint32_t total_channels;
		uint32_t* current_sample;
		
		FontHandle font_handle;
		Color background_color;
		Color foreground_color;
		
		float font_height;
		
		enum ChannelType
		{
			ChannelColor,
			ChannelMin,
			ChannelMax,
			ChannelTotal
		};
		
	private:
		void draw_float(Renderer* renderer, float value, const Point& pt, const gui::Color& color);
		
	public:
		LIBRARY_EXPORT Graph(Panel* parent);
		LIBRARY_EXPORT virtual ~Graph();
		
		LIBRARY_EXPORT void create_samples(uint32_t max_samples, uint32_t max_channels);
		LIBRARY_EXPORT void configure_channel(uint32_t channel_index, const gui::Color& color/*, const gui::Color& min, const gui::Color& max*/);
		LIBRARY_EXPORT void enable_baseline(bool enabled, float value = 0.0f, const gui::Color& color = gui::Color());
		LIBRARY_EXPORT bool channel_in_range(uint32_t channel_index) const;
		LIBRARY_EXPORT void record_value(float value, uint32_t channel_index);
		LIBRARY_EXPORT void set_range(float min_range, float max_range);
			
		LIBRARY_EXPORT void set_font(Compositor* compositor, const char* path);
		LIBRARY_EXPORT virtual void set_background_color(const Color& color);
		LIBRARY_EXPORT virtual void set_foreground_color(const Color& color);
		
		// Panel overrides
		LIBRARY_EXPORT virtual void render(Rect& frame, Compositor* compositor, Renderer* renderer, Style* style);

		LIBRARY_EXPORT virtual bool can_move() const { return true; }
	}; // Graph
} // namespace gui

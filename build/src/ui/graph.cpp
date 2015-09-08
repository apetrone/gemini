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
#include "ui/graph.h"
#include "ui/renderer.h"
#include "ui/compositor.h"

#include <string.h> // for memset

namespace gui
{
	void Graph::draw_float(Renderer* renderer, float value, const Point& pt, const gui::Color& color, gui::render::CommandList& render_commands)
	{
		char string_value[16] = {0};
		sprintf(string_value, "%2.2f", value);
		Rect bounds;

		Rect string_bounds;
		renderer->font_measure_string(font_handle, string_value, string_bounds);

		bounds.origin = pt + glm::vec2(0, glm::max(font_height, string_bounds.height()));
		render_commands.add_font(font_handle, string_value, bounds, color);
	}



	Graph::Graph(Panel* parent) : Panel(parent)
	{
		values = 0;
		
		range_min = -1;
		range_max = -1;
		
		last_point = 0;
		channel_colors = 0;
		
		baseline_value = 0.0f;
		
		flags = 0;
		total_samples = 0;
		total_channels = 0;
		current_sample = 0;
		
		background_color = Color(0, 0, 0, 255);
		foreground_color = Color(255, 255, 255, 255);

		show_baseline = false;
	}
	
	Graph::~Graph()
	{
		// purge data
		
		if(values)
		{
			delete [] values;
			values = 0;
		}
		
		if (channel_colors)
		{
			delete [] channel_colors;
			channel_colors = 0;
		}
		
		if (current_sample)
		{
			delete [] current_sample;
			current_sample = 0;
		}
		
		if (last_point)
		{
			delete [] last_point;
			last_point = 0;
		}
	}
	
	
	void Graph::create_samples(uint32_t max_samples, uint32_t max_channels)
	{
		total_samples = max_samples;
		total_channels = max_channels;
		
		// allocate 'current' pointers for each channel
		current_sample = new uint32_t[max_channels];
		memset(current_sample, 0, max_channels * sizeof(uint32_t));
		
		// allocate samples
		values = new float[ max_samples * max_channels ];
		memset(values, 0, sizeof(float) * (max_samples*max_channels));
		
		// allocate channel colors
		channel_colors = new gui::Color[ max_channels * ChannelTotal ];
		
		last_point = new Point[ max_channels ];
		memset(last_point, 0, sizeof(Point) * max_channels);
	}
	
	void Graph::configure_channel(uint32_t channel_index, const gui::Color& color/*, const gui::Color& min_color, const gui::Color& max_color*/)
	{
		if (!channel_in_range(channel_index))
		{
			return;
		}
		
		gui::Color* colors = &channel_colors[ChannelTotal*channel_index];
		colors[ChannelColor] = color;
//		colors[ChannelMin] = min_color;
//		colors[ChannelMax] = max_color;
	}
	
	void Graph::enable_baseline(bool enabled, float value, const gui::Color& color)
	{
		show_baseline = enabled;
		if (enabled)
		{
			baseline_value = value;
			baseline_color = color;
		}
	}
	
	bool Graph::channel_in_range(uint32_t channel_index) const
	{
		return (channel_index < total_channels);
	}
	
	void Graph::record_value(float value, uint32_t channel_index)
	{
		if (!channel_in_range(channel_index))
		{
			return;
		}
		
		
		values[ (channel_index*total_samples) + current_sample[channel_index] ] = value;
		++current_sample[channel_index];
		
		if (current_sample[channel_index] >= total_samples)
		{
			current_sample[channel_index] = 0;
		}
	}
	
	void Graph::set_range(float min_range, float max_range)
	{
		range_min = min_range;
		range_max = max_range;
	}
	
	void Graph::set_font(const char* filename, size_t pixel_size)
	{
		Compositor* compositor = get_compositor();
		font_handle = compositor->get_resource_cache()->create_font(filename, pixel_size);
		assert(font_handle.is_valid());

		size_t height;
		int ascender;
		int descender;
		compositor->get_renderer()->font_metrics(font_handle, height, ascender, descender);
		font_height = static_cast<float>(ascender + descender);
	}
	
	void Graph::set_background_color(const Color& color)
	{
		background_color = color;
	}
	
	void Graph::set_foreground_color(const Color& color)
	{
		foreground_color = color;
	}
	
	void Graph::render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands)
	{
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			background_color
		);

		const Rect& frame = bounds;
		float dx = (float)frame.size.width / (float)total_samples;
		float y = frame.origin.y;
		float height = frame.size.height;
		float vertical_scale = range_max - range_min;
		float yoffset = (range_min / vertical_scale) * height;
		float baseline_y = ((range_min-baseline_value) / vertical_scale) * height;

		if (show_baseline)
		{
			// draw a line
			Point start(frame.origin.x, baseline_y+y+height);
			Point end(frame.origin.x+frame.size.width, baseline_y+y+height);

			render_commands.add_line(start, end, baseline_color);
		}

		for( unsigned int current_channel = 0; current_channel < total_channels; ++current_channel )
		{
			// draw the most current information on the right
			float cx = frame.origin.x;
			
			// left to right graph: (sample_delta=1, dx = -dx, cx = right)
			// right to left: (sample_delta=-1, cx=left)
			int sample_delta = 1;
			
			unsigned int sample_id = current_sample[ current_channel ];
			Color* colors = &channel_colors[ ChannelTotal * current_channel ];
			
			for(uint32_t i = 0; i < total_samples; ++i)
			{
				Color color = colors[ ChannelColor ];
				float sample_value = values[ (current_channel * total_samples) + ((sample_id) % total_samples) ];
	
				if ( sample_value < range_min )
				{
					sample_value = range_min;
				}
				
				if ( sample_value < baseline_value )
				{
//					color = colors[ ChannelMin ];
				}
				else if ( sample_value > range_max )
				{
					sample_value = range_max;
//					color = colors[ ChannelMax ];
				}
				

				float sample_y = (sample_value / vertical_scale);
				float outvalue = yoffset+y+height-(sample_y*height);

				if ( i > 0 )
				{
					Point current( cx, outvalue );
					render_commands.add_line(last_point[current_channel], current, color);
					last_point[ current_channel ] = current;

				}
				else
				{
					last_point[ current_channel ].x = cx;
					last_point[ current_channel ].y = outvalue;
				}

				cx += dx;
				sample_id += sample_delta;
			}
		}
		
		
		
		// draw text
		Point left_margin(frame.origin.x + 2, frame.origin.y);

		draw_float(renderer, range_max, left_margin, foreground_color, render_commands);

		if (show_baseline)
		{
			left_margin.y = baseline_y + y + height - (font_height/2.0f);
			draw_float(renderer, baseline_value, left_margin, foreground_color, render_commands);
		}


		left_margin.y = y + height - font_height;
		draw_float(renderer, range_min, left_margin, foreground_color, render_commands);
	}

} // namespace gui

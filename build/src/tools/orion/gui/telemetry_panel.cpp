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

#include "telemetry_panel.h"
#include <ui/compositor.h>

namespace gemini
{

	void TelemetryPanel::update(gui::Compositor* compositor, float delta_seconds)
	{
		min_bar_width = (get_client_size().width / static_cast<float>(TELEMETRY_MAX_VIEWER_FRAMES));
		gui::Panel::update(compositor, delta_seconds);
	}

	void TelemetryPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		gui::Painter painter(this, render_commands);

		render_geometry(render_commands, background_color);
		render_capture_rect(render_commands);
		render_background(render_commands);

		uint32_t width = get_client_size().width;
		float rect_width = bar_width;

		const gemini::Color current(1.0f, 1.0f, 1.0f);
		const gemini::Color normal(1.0f, 0.0f, 0.0f);
		const gemini::Color selected(1.0f, 0.5f, 0.0f);

		float client_height = get_client_size().height;
		float panel_height = get_size().height;

		const glm::mat3& tx = get_transform(0);
		uint32_t visible_frames = (width / bar_width);

		uint32_t max_scale_current_frame = adaptive_max_scale;
		adaptive_max_scale = 0;

		for (size_t index = 0; index < visible_frames; ++index)
		{
			gui::Point origin = gui::Point((index * rect_width), capture_rect.height());

			// just grab and graph the first record
			debug_record_t* record = &viewer->frames[index].records[0];

			if (viewer->frames[index].total_cycles > adaptive_max_scale)
			{
				adaptive_max_scale = viewer->frames[index].total_cycles;
			}

			float scale = viewer->frames[index].total_cycles / static_cast<float>(max_scale_current_frame);
			float rect_height = scale * static_cast<float>(client_height);

			tube[0] = gui::Point(0.0f, client_height - rect_height);
			tube[1] = gui::Point(0.0f, client_height);
			tube[2] = gui::Point(rect_width, client_height);
			tube[3] = gui::Point(rect_width, client_height - rect_height);

			gemini::Color current_color = normal;
			if (selected_frame != -1 && index == selected_frame)
			{
				current_color = selected;
			}
			else if (index == viewer->current_index)
			{
				current_color = current;
			}

			painter.add_rectangle(
				origin + tube[0],
				origin + tube[1],
				origin + tube[2],
				origin + tube[3],
				gui::render::WhiteTexture,
				current_color
			);
		}

		render_children(compositor, renderer, render_commands);
	}

	void TelemetryPanel::handle_event(gui::EventArgs& args)
	{
		last_position = args.local;

		// Allow the cursor to still drag by the title bar.
		// TODO: Determine WHERE the capture was made. If it wasn't made in
		// the capture rect, the inherited Panel shouldn't handle it. This panel should.
		Panel::handle_event(args);
		if (args.handled || point_in_capture_rect(args.local))
		{
			return;
		}

		if (args.type == gui::Event_CursorDrag || args.type == gui::Event_CursorButtonPressed)
		{
			if (args.type == gui::Event_CursorButtonPressed)
			{
				args.compositor->set_capture(this, args.cursor_button);
			}

			int next_frame = (((int)args.local.x) - 1) / bar_width;

			selected_frame = next_frame;

			LOGV("selected frame: %i\n", next_frame);
			if (selected_frame >= 0 && selected_frame < TELEMETRY_MAX_VIEWER_FRAMES)
			{
				info_panel->set_visible(true);
				float x_offset = args.cursor.x;
				float x_origin = get_origin().x;

				if ((x_origin + x_offset + info_panel->get_size().width) > args.compositor->get_size().width)
				{
					x_offset = (args.compositor->get_size().width - info_panel->get_size().width - x_origin);
				}
				gui::Point info_panel_offset = args.compositor->compositor_to_local(gui::Point(x_offset, 0.0f));

				info_panel->set_origin(info_panel_offset.x, capture_rect.height());

				String profile_block;
				for (size_t index = 0; index < TELEMETRY_MAX_RECORDS_PER_FRAME; ++index)
				{
					debug_record_t* record = &viewer->frames[selected_frame].records[index];
					if (record->cycles > 0)
					{
						profile_block += core::str::format(
							"[%s:%i - %s] Cycles: %i\n",
							record->filename,
							record->line_number,
							record->function,
							record->cycles);
					}
				}
				//profile_block += core::str::format("Total Cycles: %i", viewer->frames[selected_frame].total_cycles);
				info_panel->set_profile_block(profile_block.c_str());

				String variable_block;
				for (size_t index = 0; index < TELEMETRY_MAX_VARIABLES; ++index)
				{
					debug_var_t* variable = &viewer->frames[selected_frame].variables[index];
					if (variable->name[0] > 0)
					{
						if (variable->type == DEBUG_RECORD_TYPE_FLOAT)
						{
							float* value = reinterpret_cast<float*>(variable->data);
							variable_block += core::str::format("[%i] \"%s\": %2.2f\n", index, variable->name, *value);
						}
						else if (variable->type == DEBUG_RECORD_TYPE_FLOAT2)
						{
							glm::vec2* value = reinterpret_cast<glm::vec2*>(variable->data);
							variable_block += core::str::format("[%i] \"%s\": [%2.2f, %2.2f]\n", index, variable->name, value->x, value->y);
						}
						else if (variable->type == DEBUG_RECORD_TYPE_FLOAT3)
						{
							glm::vec3* value = reinterpret_cast<glm::vec3*>(variable->data);
							variable_block += core::str::format("[%i] \"%s\": [%2.2f, %2.2f, %2.2f]\n", index, variable->name, value->x, value->y, value->z);
						}
						else if (variable->type == DEBUG_RECORD_TYPE_FLOAT4)
						{
							glm::vec4* value = reinterpret_cast<glm::vec4*>(variable->data);
							variable_block += core::str::format("[%i] \"%s\": [%2.2f, %2.2f, %2.2f, %2.2f]\n", index, variable->name, value->x, value->y, value->z, value->w);
						}
						else if (variable->type == DEBUG_RECORD_TYPE_UINT32)
						{
							uint32_t* value = reinterpret_cast<uint32_t*>(variable->data);
							variable_block += core::str::format("[%i] \"%s\": %u\n", index, variable->name, *value);
						}
						else if (variable->type == DEBUG_RECORD_TYPE_INT32)
						{
							int32_t* value = reinterpret_cast<int32_t*>(variable->data);
							variable_block += core::str::format("[%i] \"%s\": %i\n", index, variable->name, *value);
						}
					}
				}
				info_panel->set_variable_block(variable_block.c_str());
			}
			else
			{
				info_panel->set_visible(false);
			}

			args.handled = true;
		}
		else if (args.type == gui::Event_CursorScroll)
		{
			bar_width += args.wheel * 2.0f;
			LOGV("bar width is %2.2f\n", bar_width);
			if (bar_width < min_bar_width)
			{
				bar_width = min_bar_width;
			}
		}
	}
} // namespace gemini
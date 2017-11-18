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
#pragma once

#include <ui/panel.h>
#include <ui/label.h>
#include <runtime/debug_event.h>

namespace gemini
{
	class TelemetryPanel : public gui::Panel
	{
	public:

		gui::Point tube[4];

		telemetry_viewer* viewer;

		gui::Point last_position;
		int32_t selected_frame;
		uint32_t adaptive_max_scale;

		float bar_width;
		float min_bar_width;

		class TelemetryInfo : public gui::Panel
		{
			gui::Label* profile_block;
			gui::Label* variable_block;

		public:
			TelemetryInfo(gui::Panel* parent)
				: gui::Panel(parent)
			{
				profile_block = new gui::Label(this);
				profile_block->set_size(250, 100);
				profile_block->set_font("debug", 16);
				profile_block->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.5f));
				profile_block->set_foreground_color(gemini::Color(0.0f, 1.0f, 0.0f));
				profile_block->set_name("profile_block");

				variable_block = new gui::Label(this);
				variable_block->set_size(250, 100);
				variable_block->set_origin(0, 100);
				variable_block->set_font("debug", 16);
				variable_block->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.5f));
				variable_block->set_foreground_color(gemini::Color(0.0f, 1.0f, 1.0f));
				variable_block->set_name("variable_block");
			}

			void set_profile_block(const char* text)
			{
				profile_block->set_text(text);
			}

			void set_variable_block(const char* text)
			{
				variable_block->set_text(text);
			}

			virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
			{
				render_geometry(render_commands, background_color);
				//render_capture_rect(render_commands);
				render_background(render_commands);
				render_children(compositor, renderer, render_commands);
			}
		};


		TelemetryInfo* info_panel;


		TelemetryPanel(gui::Panel* parent, telemetry_viewer* telemetry_viewer)
			: gui::Panel(parent)
			, viewer(telemetry_viewer)
		{
			set_background_color(gemini::Color(0.5f, 0.5f, 0.5f));

			tube[0] = gui::Point(0.0f, 0.0f);
			tube[1] = gui::Point(0.0f, 50.0f);
			tube[2] = gui::Point(50.0f, 50.0f);
			tube[3] = gui::Point(50.0f, 0.0f);

			flags |= gui::Panel::Flag_CanMove;
			selected_frame = -1;

			bar_width = 6.0f;

			info_panel = new TelemetryInfo(this);
			info_panel->set_size(250, 200);
			info_panel->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.5f));
			adaptive_max_scale = 100;
		}

		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
		virtual void handle_event(gui::EventArgs& args) override;
	};

} // namespace gemini


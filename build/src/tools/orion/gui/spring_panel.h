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

#include "ui/panel.h"

namespace gemini
{
	class SpringPanel : public gui::Panel
	{
	public:

		gui::Point tube[4];

		struct Object
		{
			gui::Point position;
			gui::Point velocity;
		};

		float k;
		float x;
		Object box;

		gui::Point target;

		SpringPanel(gui::Panel* parent)
			: gui::Panel(parent)
		{
			set_background_color(gemini::Color(0.5f, 0.5f, 0.5f));

			tube[0] = gui::Point(0.0f, 0.0f);
			tube[1] = gui::Point(0.0f, 50.0f);
			tube[2] = gui::Point(50.0f, 50.0f);
			tube[3] = gui::Point(50.0f, 0.0f);

			box.position = gui::Point(0.0f, 0.0f);
			box.velocity = gui::Point(0.0f, 0.0f);
			target = gui::Point(50.0f, 0.0f);
		}

		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
	};
} // namespace gemini
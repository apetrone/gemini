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
#include "ui/events.h"
#include "ui/utils.h"

#include <string>

namespace gui
{
	class Slider : public gui::Panel
	{
	public:
		Slider(Panel* parent);

		LIBRARY_EXPORT virtual void handle_event(gui::EventArgs& args) override;
			
		LIBRARY_EXPORT virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		LIBRARY_EXPORT virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands);

		LIBRARY_EXPORT virtual void set_value(float new_value);
		LIBRARY_EXPORT virtual float get_value() const { return current_value; }

		gui::DelegateHandler<float> on_value_changed;

	protected:

		Point get_left_edge();
		Point get_right_edge();

		//
		float current_value;

		Panel* drag_handle;

		float drag_handle_width;
	};
} // namespace gui
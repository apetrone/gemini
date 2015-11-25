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

#include "ui/label.h"
#include "ui/events.h"


namespace gui
{
	class Compositor;
	class Renderer;

	class Button : public Label
	{
	public:
		LIBRARY_EXPORT Button(Panel* parent);
		LIBRARY_EXPORT virtual ~Button();

		LIBRARY_EXPORT virtual void set_hover_color(const core::Color& hover) { hover_color = hover; }
		LIBRARY_EXPORT virtual void set_pressed_color(const core::Color& pressed) { pressed_color = pressed; }

		LIBRARY_EXPORT virtual void handle_event(EventArgs& args);

		LIBRARY_EXPORT virtual void update(Compositor* compositor, float delta_seconds) override;
		LIBRARY_EXPORT virtual void render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands);

		LIBRARY_EXPORT virtual bool is_button() const { return true; }

		DelegateHandler<EventArgs&> on_click;

	protected:
		core::Color hover_color;
		core::Color pressed_color;
		core::Color current_color;

		uint32_t state;
	}; // struct Button
} // namespace gui

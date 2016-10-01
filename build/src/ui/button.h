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

#include <core/typespec.h>

namespace gui
{
	class Compositor;
	class Renderer;

	class Button : public Panel
	{
		TYPESPEC_DECLARE_CLASS(Button, Panel);

	public:
		Button(Panel* parent);
		virtual ~Button();

		virtual void set_hover_color(const gemini::Color& hover) { hover_color = hover; }
		virtual void set_pressed_color(const gemini::Color& pressed) { pressed_color = pressed; }

		virtual void handle_event(EventArgs& args) override;

		virtual void update(Compositor* compositor, float delta_seconds) override;
		virtual void render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands) override;

		virtual void set_font(const char* filename, size_t pixel_size);
		virtual void set_text(const std::string& text);
		virtual bool is_button() const override { return true; }

		gemini::Delegate<void(EventArgs&)> on_click;
		gemini::Delegate<void(EventArgs&)> on_pressed;

	protected:
		gemini::Color hover_color;
		gemini::Color pressed_color;
		gemini::Color current_color;
		std::string text;
		FontHandle font_handle;
		Point text_origin;
		int32_t font_height;
		uint32_t state;
	}; // struct Button
} // namespace gui

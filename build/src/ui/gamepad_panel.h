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

namespace gemini
{
	class JoystickInput;
}


namespace gui
{
	class Compositor;
	class Renderer;

	namespace render
	{
		struct CommandList;
	}

	class JoystickAxisPanel : public gui::Panel
	{
	public:
		JoystickAxisPanel(gui::Panel* parent);
		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;

		float value;
		float duration;
	};

	// ----------------------------------------------------------------------------
	class JoystickAnalogPanel : public gui::Panel
	{
	public:
		JoystickAnalogPanel(gui::Panel* parent);
		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;

		float left;
		float right;
		float up;
		float down;
		float thumb_button;
	};

	// ----------------------------------------------------------------------------
	class GamepadPanel : public gui::Panel
	{
	public:
		gui::Size BUMPER;
		gui::Size BUTTON_SIZE;
		gui::Size ANALOG_SIZE;

		GamepadPanel(gui::Panel* parent);
		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;

		void set_from_joystick(gemini::JoystickInput& joystick);

	private:
		JoystickAxisPanel* left_bumper;
		JoystickAxisPanel* right_bumper;

		JoystickAxisPanel* left_trigger;
		JoystickAxisPanel* right_trigger;

		JoystickAxisPanel* left_button;
		JoystickAxisPanel* right_button;
		JoystickAxisPanel* up_button;
		JoystickAxisPanel* down_button;

		JoystickAxisPanel* x_button;
		JoystickAxisPanel* y_button;
		JoystickAxisPanel* a_button;
		JoystickAxisPanel* b_button;

		JoystickAxisPanel* back_button;
		JoystickAxisPanel* start_button;

		JoystickAnalogPanel* left_stick;
		JoystickAnalogPanel* right_stick;
	};
} // namespace gui



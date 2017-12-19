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

#include <ui/gamepad_panel.h>
#include <ui/compositor.h>
#include <renderer/renderer.h>

#include <runtime/inputstate.h>

#include <platform/kernel.h>

#include <platform/input.h>

using namespace gemini;

namespace gui
{
	JoystickAxisPanel::JoystickAxisPanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
		value = 0.0f;
		duration = 0.0f;
	}

	void JoystickAxisPanel::update(gui::Compositor* compositor, float delta_seconds)
	{
		gui::Panel::update(compositor, delta_seconds);
	}

	void JoystickAxisPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		const gemini::Color UP_COLOR(0.1f, 0.1f, 0.1f);
		const gemini::Color FULL_COLOR(1.0f, 0.0f, 0.0f);
		const gemini::Color HELD_COLOR(1.0f, 1.0f, 1.0f);

		if (value <= 0.1f)
		{
			set_background_color(UP_COLOR);
		}
		else if (value > 0.0f && value < 1.0f)
		{
			set_background_color(interpolate(UP_COLOR, FULL_COLOR, value));
		}
		else if (value >= 0.99f)
		{
			set_background_color(FULL_COLOR);
		}

		if (value >= 0.99f && duration > 0.5f)
		{
			set_background_color(HELD_COLOR);
		}

		gui::Panel::render(compositor, renderer, render_commands);
	}

	// ----------------------------------------------------------------------------


	JoystickAnalogPanel::JoystickAnalogPanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
	}

	void JoystickAnalogPanel::update(gui::Compositor* compositor, float delta_seconds)
	{
		gui::Panel::update(compositor, delta_seconds);
	}

	void JoystickAnalogPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		const gemini::Color UP_COLOR(0.1f, 0.1f, 0.1f);
		const gemini::Color FULL_COLOR(1.0f, 0.0f, 0.0f);

		if (thumb_button <= 0.1f)
		{
			set_background_color(UP_COLOR);
		}
		else if (thumb_button > 0.0f && thumb_button < 1.0f)
		{
			set_background_color(interpolate(UP_COLOR, FULL_COLOR, thumb_button));
		}
		else if (thumb_button >= 0.99f)
		{
			set_background_color(FULL_COLOR);
		}

		gui::Painter painter(this, render_commands);

		gui::Point left_cursor(
			0.5 + (0.5 * (right - left)),
			0.5 + (0.5 * -(up - down)));

		gemini::Color left_colors[] = {
			gemini::Color(1.0f, 1.0f, 0.0f),
			gemini::Color(1.0f, 0.0f, 1.0f)
		};

		gui::Point left_vertices[] = {
			gui::Point(get_size().width * left_cursor.x, 0),
			gui::Point(get_size().width * left_cursor.x, get_size().height),

			gui::Point(0, get_size().height * left_cursor.y),
			gui::Point(get_size().width, get_size().height * left_cursor.y)
		};

		gui::Panel::render(compositor, renderer, render_commands);
		painter.add_lines(2, left_vertices, left_colors);
	}



	GamepadPanel::GamepadPanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
		BUMPER = gui::Size(100, 30);
		BUTTON_SIZE = gui::Size(40, 40);
		ANALOG_SIZE = gui::Size(80, 80);

		left_bumper = new JoystickAxisPanel(this);
		left_bumper->set_size(BUMPER);

		right_bumper = new JoystickAxisPanel(this);
		right_bumper->set_size(BUMPER);

		left_trigger = new JoystickAxisPanel(this);
		left_trigger->set_size(BUMPER);

		right_trigger = new JoystickAxisPanel(this);
		right_trigger->set_size(BUMPER);

		left_button = new JoystickAxisPanel(this);
		left_button->set_size(BUTTON_SIZE);

		right_button = new JoystickAxisPanel(this);
		right_button->set_size(BUTTON_SIZE);

		up_button = new JoystickAxisPanel(this);
		up_button->set_size(BUTTON_SIZE);

		down_button = new JoystickAxisPanel(this);
		down_button->set_size(BUTTON_SIZE);

		x_button = new JoystickAxisPanel(this);
		x_button->set_size(BUTTON_SIZE);

		y_button = new JoystickAxisPanel(this);
		y_button->set_size(BUTTON_SIZE);

		a_button = new JoystickAxisPanel(this);
		a_button->set_size(BUTTON_SIZE);

		b_button = new JoystickAxisPanel(this);
		b_button->set_size(BUTTON_SIZE);

		back_button = new JoystickAxisPanel(this);
		back_button->set_size(BUTTON_SIZE);

		start_button = new JoystickAxisPanel(this);
		start_button->set_size(BUTTON_SIZE);

		left_stick = new JoystickAnalogPanel(this);
		left_stick->set_size(ANALOG_SIZE);

		right_stick = new JoystickAnalogPanel(this);
		right_stick->set_size(ANALOG_SIZE);
	}

	void GamepadPanel::update(gui::Compositor* compositor, float delta_seconds)
	{
		gui::Panel::update(compositor, delta_seconds);

		const float top_height = 15;

		left_trigger->set_origin(10, top_height);
		right_trigger->set_origin(get_size().width - BUMPER.width - 10, top_height);

		left_bumper->set_origin(10, top_height + 10 + 30);
		right_bumper->set_origin(get_size().width - BUMPER.width - 10, top_height + 10 + 30);

		const gui::Point left_offset(110, 100);
		left_button->set_origin(left_offset + gui::Point(-50, 50));
		up_button->set_origin(left_offset + gui::Point(0, 0));
		down_button->set_origin(left_offset + gui::Point(0, 100));
		right_button->set_origin(left_offset + gui::Point(50, 50));

		const gui::Point right_offset(get_size().width - 150, 100);
		x_button->set_origin(right_offset + gui::Point(-50, 50));
		y_button->set_origin(right_offset + gui::Point(0, 0));
		a_button->set_origin(right_offset + gui::Point(0, 100));
		b_button->set_origin(right_offset + gui::Point(50, 50));

		back_button->set_origin(185, 75);
		start_button->set_origin(get_size().width - 225, 75);

		left_stick->set_origin(220, 220);
		right_stick->set_origin(get_size().width - 260, 220);
	}


	void GamepadPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		gui::Panel::render(compositor, renderer, render_commands);
	}

	void GamepadPanel::set_from_joystick(gemini::JoystickInput& joystick)
	{
		left_trigger->value			= joystick.get_button(GAMEPAD_BUTTON_L2).normalized_value();
		right_trigger->value		= joystick.get_button(GAMEPAD_BUTTON_R2).normalized_value();

		left_bumper->value			= joystick.get_button(GAMEPAD_BUTTON_LEFTSHOULDER).normalized_value();
		left_bumper->duration		= joystick.get_button(GAMEPAD_BUTTON_LEFTSHOULDER).held_ticks(kernel::parameters().current_physics_tick) * kernel::parameters().step_interval_seconds;
		right_bumper->value			= joystick.get_button(GAMEPAD_BUTTON_RIGHTSHOULDER).normalized_value();

		left_button->value			= joystick.get_button(GAMEPAD_BUTTON_DPAD_LEFT).normalized_value();
		up_button->value			= joystick.get_button(GAMEPAD_BUTTON_DPAD_UP).normalized_value();
		right_button->value			= joystick.get_button(GAMEPAD_BUTTON_DPAD_RIGHT).normalized_value();
		down_button->value			= joystick.get_button(GAMEPAD_BUTTON_DPAD_DOWN).normalized_value();

		x_button->value				= joystick.get_button(GAMEPAD_BUTTON_X).normalized_value();
		y_button->value				= joystick.get_button(GAMEPAD_BUTTON_Y).normalized_value();
		a_button->value				= joystick.get_button(GAMEPAD_BUTTON_A).normalized_value();
		b_button->value				= joystick.get_button(GAMEPAD_BUTTON_B).normalized_value();

		back_button->value			= joystick.get_button(GAMEPAD_BUTTON_BACK).normalized_value();
		start_button->value			= joystick.get_button(GAMEPAD_BUTTON_START).normalized_value();

		left_stick->left			= joystick.get_button(GAMEPAD_STICK0_AXIS_LEFT).normalized_value();
		left_stick->right			= joystick.get_button(GAMEPAD_STICK0_AXIS_RIGHT).normalized_value();
		left_stick->up				= joystick.get_button(GAMEPAD_STICK0_AXIS_UP).normalized_value();
		left_stick->down			= joystick.get_button(GAMEPAD_STICK0_AXIS_DOWN).normalized_value();
		left_stick->thumb_button	= joystick.get_button(GAMEPAD_BUTTON_L3).normalized_value();

		right_stick->left			= joystick.get_button(GAMEPAD_STICK1_AXIS_LEFT).normalized_value();
		right_stick->right			= joystick.get_button(GAMEPAD_STICK1_AXIS_RIGHT).normalized_value();
		right_stick->up				= joystick.get_button(GAMEPAD_STICK1_AXIS_UP).normalized_value();
		right_stick->down			= joystick.get_button(GAMEPAD_STICK1_AXIS_DOWN).normalized_value();
		right_stick->thumb_button	= joystick.get_button(GAMEPAD_BUTTON_R3).normalized_value();
	}
} // namespace gui
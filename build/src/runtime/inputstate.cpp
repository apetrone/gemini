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

#include <runtime/inputstate.h>
#include <core/logging.h>

#include <platform/platform.h>
#include <platform/kernel_events.h>


namespace gemini
{
	// ---------------------------------------------------------------------
	// devices
	// ---------------------------------------------------------------------


	//
	// KeyboardInput

	void KeyboardInput::reset()
	{
		memset(&keys, 0, sizeof(ButtonState) * BUTTON_COUNT);
	} // reset

	void KeyboardInput::update()
	{
		for (unsigned int i = 0; i < BUTTON_COUNT; ++i)
		{
			keys[i].update();
		}
	} // update

	void KeyboardInput::inject_key_event(int key, bool is_down)
	{
		Button button = static_cast<Button>(key);
		assert(button <= BUTTON_COUNT);
		ButtonState* b = &keys[button];
		b->update_state(is_down);
	} // inject_key_event

//	bool KeyboardInput::is_down(input::Button key)
//	{
//		ButtonState* b;
//		assert( key < BUTTON_COUNT );
//		b = &keys[ key ];
//		return b->is_down();
//	} // is_down
//
//	bool KeyboardInput::was_released(input::Button key )
//	{
//		ButtonState* b;
//		assert( key < BUTTON_COUNT );
//		b = &keys[ key ];
//		return b->was_released();
//	} // was_released

	//
	// MouseInput

	void MouseInput::reset()
	{
		memset(&buttons, 0, sizeof(ButtonState) * MOUSE_COUNT);
		window_coords[0] = window_coords[1] = 0;
	} // reset

	void MouseInput::update()
	{
		for (unsigned int i = 0; i < MOUSE_COUNT; ++i)
		{
			buttons[MOUSE_LEFT + i].update();
		}
	} // update


	void MouseInput::inject_mouse_move(int absolute_x, int absolute_y)
	{
		window_coords[0] = absolute_x;
		window_coords[1] = absolute_y;

		//		LOGV("inject move: %i %i\n", absolute_x, absolute_y);
	} // inject_mouse_move

	void MouseInput::inject_mouse_delta(int dx, int dy)
	{
		cursor_delta[0] += dx;
		cursor_delta[1] += dy;
	}

	void MouseInput::inject_mouse_button(MouseButton button, bool is_down)
	{
		assert(button < MOUSE_COUNT && button >= 0);
		buttons[button].update_state(is_down);
	} // inject_mouse_button

	void MouseInput::inject_mouse_wheel(int direction)
	{
		wheel_direction = direction;
	} // inject_mouse_wheel

	void MouseInput::mouse_position(int& x, int& y)
	{
		x = window_coords[0];
		y = window_coords[1];
	} // mouse_position

	void MouseInput::mouse_delta(int& dx, int& dy)
	{
		dx = cursor_delta[0];
		dy = cursor_delta[1];
	} // mouse_delta

	const ButtonState& MouseInput::get_button(const gemini::MouseButton button) const
	{
		return buttons[button];
	} // get_button

	//
	// JoystickInput
	void JoystickInput::reset()
	{
		memset(&axes, 0, sizeof(ButtonState) * GAMEPAD_BUTTON_COUNT);
		haptic_force = 0;
		haptic_duration_seconds = 0;
	}

	void JoystickInput::update()
	{
		for (uint8_t axis_index = 0; axis_index < GAMEPAD_BUTTON_COUNT; ++axis_index)
		{
			axes[axis_index].update();
		}

		platform::joystick_set_force(static_cast<uint32_t>(index), haptic_force);
	}

	const ButtonState& JoystickInput::get_button(gemini::GamepadButton button) const
	{
		assert(button < GAMEPAD_BUTTON_COUNT);
		return axes[button];
	} // get_button


	void input_message_to_inputstate(const InputMessage& message, InputState& state)
	{
		switch (message.type)
		{
		case InputMessage::Keyboard:
			//if (message.button == BUTTON_F2)
			//{
			//	gemini::engine::instance()->terminate_application();
			//}
			state.keyboard().inject_key_event(message.button, message.params[0]);
			break;

		case InputMessage::Mouse:
			state.mouse().inject_mouse_button(static_cast<MouseButton>(message.button), message.params[0]);
			break;

		case InputMessage::MouseMove:
			state.mouse().inject_mouse_move(message.params[0], message.params[1]);
			break;

		case InputMessage::MouseDelta:
			state.mouse().inject_mouse_delta(message.params[0], message.params[1]);
			break;

		case InputMessage::MouseWheel:
			state.mouse().inject_mouse_wheel(message.button);
			//state.mouse().inject_mouse_move(message.params[0], message.params[1]);
			//state.mouse().inject_mouse_delta(message.params[2], message.params[3]);
			break;

		case InputMessage::GamePadConnected:
		{
			LOGV("gamepad [%i] connected\n", message.params[0]);
			JoystickInput& joystick = state.joystick_by_index(message.params[0]);
			joystick.state = GEMINI_JOYSTICK_CONNECTED;
			break;
		}

		case InputMessage::GamePadDisconnected:
		{
			JoystickInput& joystick = state.joystick_by_index(message.params[0]);
			LOGV("gamepad [%i] disconnected\n", message.params[0]);
			joystick.state = GEMINI_JOYSTICK_DISCONNECTED;
			break;
		}

		case InputMessage::GamePadButton:
		case InputMessage::GamePadAxis:
		{
			JoystickInput& joystick = state.joystick_by_index(message.params[0]);
			//LOGV("gamepad [%i] button: %s (%i), value: %i\n", message.params[0], gemini::joystick_button_name(message.params[1]), message.params[1], message.params[2]);
			joystick.axes[message.params[1]].update_state(message.params[2]);
			break;
		}

		case InputMessage::System:
		{
			// Ignore system events.
			break;
		}

		default:
			// Unhandled input event!
			assert(0);
			break;
		}
	} // input_message_to_inputstate


	void InputEventRelay::queue(const gemini::InputMessage& message)
	{
		messages.push_back(message);
	} // queue

	void InputEventRelay::queue(const kernel::SystemEvent& event, uint64_t current_tick)
	{
		InputMessage out;
		out.timestamp = current_tick;
		out.type = InputMessage::System;
		if (event.subtype == kernel::WindowGainFocus)
		{
			out.params[0] = 1;
		}
		else if (event.subtype == kernel::WindowLostFocus)
		{
			out.params[1] = 1;
		}
		else
		{
			LOGW("Missing system event type!\n");
		}
		messages.push_back(out);
	} // queue

	void InputEventRelay::queue(const kernel::KeyboardEvent& event, uint64_t current_tick)
	{
		InputMessage out;
		out.timestamp = current_tick;
		out.type = InputMessage::Keyboard;
		out.button = event.key;
		out.params[0] = event.is_down;
		out.params[1] = event.modifiers;
		messages.push_back(out);
	} // queue

	void InputEventRelay::queue(const kernel::MouseEvent& event, uint64_t current_tick)
	{
		InputMessage out;
		out.timestamp = current_tick;
		switch (event.subtype)
		{
		case kernel::MouseButton:
			out.type = InputMessage::Mouse;
			out.button = event.button;
			out.params[0] = event.is_down;
			break;

		case kernel::MouseMoved:
			out.type = InputMessage::MouseMove;
			out.params[0] = event.mx;
			out.params[1] = event.my;
			break;

		case kernel::MouseDelta:
			out.type = InputMessage::MouseDelta;
			out.params[0] = event.dx;
			out.params[1] = event.dy;
			break;

		case kernel::MouseWheelMoved:
			out.type = InputMessage::MouseWheel;
			out.button = event.wheel_direction;
			out.params[0] = event.mx;
			out.params[1] = event.my;
			out.params[2] = event.dx;
			out.params[3] = event.dy;
			break;

		default:
			assert(0);
			break;
		}
		messages.push_back(out);
	} // queue

	void InputEventRelay::queue(const kernel::TouchEvent& event, uint64_t current_tick)
	{
		// TODO@APP: Implement touch events!
		assert(false);
	}

	void InputEventRelay::queue(const kernel::GameControllerEvent& event, uint64_t current_tick)
	{
		InputMessage out;
		out.timestamp = current_tick;
		out.params[0] = event.gamepad_id;

		switch (event.subtype)
		{
		case kernel::JoystickConnected:
			out.type = InputMessage::GamePadConnected;
			break;

		case kernel::JoystickDisconnected:
			out.type = InputMessage::GamePadDisconnected;
			break;

		case kernel::JoystickButton:
			out.type = InputMessage::GamePadButton;
			out.params[1] = event.axis_id;
			out.params[2] = event.axis_value;
			break;

		case kernel::JoystickAxisMoved:
			out.type = InputMessage::GamePadAxis;
			out.params[1] = event.axis_id;
			out.params[2] = event.axis_value;
			break;

		default:
			// Unhandled gamepad input!
			assert(0);
			break;
		}
		messages.push_back(out);
	} // queue

	void InputEventRelay::dispatch(uint64_t current_tick)
	{
		// iterate over queued messages and play until we hit the time cap
		for (size_t message_index = 0; message_index < messages.size(); ++message_index)
		{
			const gemini::InputMessage& message = messages[message_index];
			if (message.timestamp <= current_tick)
			{
				uint32_t handled_message = 0;

				for (size_t index = 0; index < handlers.size() && handled_message == 0; ++index)
				{
					input_handler& handler = handlers[index];
					if (handler(message) == 0)
					{
						// message was handled
						handled_message = 1;
					}
				}
			}
		}

		messages.resize(0);
	} // dispatch

	void InputEventRelay::add_handler(input_handler handler)
	{
		handlers.push_back(handler);
	} // add_handler
} // namespace gemini

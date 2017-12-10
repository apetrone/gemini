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

#include <core/typedefs.h>
#include <platform/input.h>
#include <core/util.h>

namespace kernel
{
	struct SystemEvent;
	struct KeyboardEvent;
	struct MouseEvent;
	struct GameControllerEvent;
	struct TouchEvent;
}

namespace gemini
{
	struct InputMessage
	{
		enum Type
		{
			Keyboard = 1,
			// button: keycode
			// params[0]: is_down
			// params[1]: keyboard modifiers

			Mouse = 8,
			// button: mouse button
			// params[0]: is_down

			MouseMove = Mouse | 16,
			// params[0]: abs mouse x
			// params[1]: abs mouse y

			MouseDelta = Mouse | 32,
			// params[0]: delta mouse x
			// params[1]: delta mouse y

			MouseWheel = Mouse | 64,
			// button: wheel delta
			// params[0]: absolute mouse x
			// params[1]: absolute mouse y
			// params[2]: delta mouse x
			// params[3]: delta mouse y

			GamePadConnected = 512,
			// params[0]: gamepad_id

			GamePadDisconnected = 1024,
			// params[0]: gamepad_id

			GamePadButton = 2048,
			// button: gamepad button
			// params[0]: gamepad_id
			// params[1]: axis_id
			// params[2]: axis_value

			GamePadAxis = 4096,
			// params[0]: gamepad_id
			// params[1]: axis_id
			// params[2]: axis_value

			System = 8192
			// params[0]: gain_focus
			// params[1]: lost_focus
		};

		uint32_t type;
		uint32_t button;
		int32_t params[4];

		// timestamp in logic ticks when this event was recorded.
		uint64_t timestamp;

		InputMessage()
			: type(0)
			, button(0)
			, timestamp(0)
		{
			params[0] = params[1] = params[2] = params[3] = 0;
		}
	}; // InputMessage
} // namespace gemini




namespace gemini
{

	class KeyboardInput
	{
		ButtonState keys[BUTTON_COUNT];

	public:
		void reset();
		void update();

		void inject_key_event(int key, bool is_down);

		inline const ButtonState& get_key(Button key) { return keys[key]; }
	}; // KeyboardInput


	class MouseInput
	{
		// absolute mouse position in window coordinates
		int window_coords[2];
		int cursor_delta[2];

	public:
		// > 0 towards screen
		// < 0 towards user
		int wheel_direction;

		ButtonState buttons[MOUSE_COUNT];

		void reset();
		void update();

		void inject_mouse_move(int absolute_x, int absolute_y);
		void inject_mouse_delta(int dx, int dy);
		void inject_mouse_button(MouseButton button_id, bool is_down);
		void inject_mouse_wheel(int direction);

		//
		// Accessors

		// retrieve the current mouse position in screen coordinates
		void mouse_position(int& x, int& y);

		void mouse_delta(int& dx, int& dy);
		void reset_delta()
		{
			cursor_delta[0] = cursor_delta[1] = 0;
		}

		const ButtonState& get_button(MouseButton key) const;
	}; // MouseInput

#if 0
	class TouchInput
	{
		struct TouchState
		{
			// (state & 1) -> isDown
			// (state & 2) -> isDragging
			unsigned int state;
			float x;
			float y;

			// [0] is the previous, [1] is current
			int xpos[2];
			int ypos[2];
		}; // TouchState

		TouchState touches[GEMINI_TOUCHES_MAX_COUNT];

	public:
		void reset();
		void update();

		void touch_began(int touchid, int x, int y);
		void touch_drag(int touchid, int x, int y);
		//	void touch_canceled( int touchid, int x, int y );
		void touch_end(int touchid, int x, int y);

		// retrieve touch from index
		void touch_at_index(TouchState & touch, int touch_id);
	}; // TouchInput
#endif

	// Joystick axes are treated as generic values [-32768, 32768].
	// axis_id maps from [0, 8]


	class JoystickInput
	{
	public:
		uint8_t index;
		uint8_t state;
		ButtonState axes[GAMEPAD_BUTTON_COUNT];
		uint16_t haptic_force;
		float haptic_duration_seconds;

		JoystickInput() :
			state(0)
		{
			index = 0;
			reset();
		}

		bool is_connected() const
		{
			return (state & GEMINI_JOYSTICK_CONNECTED) == GEMINI_JOYSTICK_CONNECTED;
		}

		const ButtonState& get_button(GamepadButton button) const;

		void reset();
		void update();
	}; // JoystickInput



	// This is a facade interface which holds the current input state
	class InputState
	{
		KeyboardInput keyboard_input;
		MouseInput mouse_input;
		//	TouchInput touch_input;
		JoystickInput joysticks[GEMINI_JOYSTICK_MAX_JOYSTICKS];

	public:

		InputState()
		{
			reset();
		}

		~InputState() {}

		inline KeyboardInput& keyboard() { return keyboard_input; }
		inline MouseInput& mouse() { return mouse_input; }
		inline JoystickInput& joystick_by_index(uint32_t index)
		{
			return joysticks[index];
		}
		//	inline TouchInput& touch() { return touch_input; }


		void update()
		{
			keyboard_input.update();
			mouse_input.update();
			for (uint8_t index = 0; index < GEMINI_JOYSTICK_MAX_JOYSTICKS; ++index)
			{
				JoystickInput& joystick = joysticks[index];
				if (joystick.is_connected())
				{
					joystick.update();
				}
			}
		}

		void reset()
		{
			keyboard_input.reset();
			mouse_input.reset();
			for (uint8_t index = 0; index < GEMINI_JOYSTICK_MAX_JOYSTICKS; ++index)
			{
				JoystickInput& joystick = joysticks[index];
				joystick.index = index;
				joystick.reset();
			}
		}
	}; // InputState



	// input related utilities/functions

	// input handler returns 1 if message was not handled and should propagate to the next
	// returns 0 if message was handled.
	typedef Delegate<int32_t(const InputMessage&)> input_handler;

	struct InputEventRelay
	{
		Array<InputMessage> messages;
		Array<input_handler> handlers;

		InputEventRelay(gemini::Allocator& allocator)
			: messages(allocator)
			, handlers(allocator)
		{
		}

		void queue(const InputMessage& message);
		void queue(const kernel::SystemEvent& event, uint64_t current_tick);
		void queue(const kernel::KeyboardEvent& event, uint64_t current_tick);
		void queue(const kernel::MouseEvent& event, uint64_t current_tick);
		void queue(const kernel::TouchEvent& event, uint64_t current_tick);
		void queue(const kernel::GameControllerEvent& event, uint64_t current_tick);

		void dispatch(uint64_t current_tick);

		void add_handler(input_handler handler);
	}; // InputEventRelay


	int16_t normalized_joystick_axis(JoystickInput& joystick, uint16_t axis_id, bool negative_half);


	void input_message_to_inputstate(const InputMessage& message, InputState& state);
} // namespace gemini

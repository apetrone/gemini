// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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

namespace platform
{
	namespace window
	{
		struct NativeWindow;
	}
}

namespace kernel
{
	class IKernel;

	//
	// Event Types
	// These are all the event types recognized and translated by the kernel.
	// The Application must be able to handle a subset of these.
	//
	enum EventType
	{
		System,

		// common to desktop applications
		Keyboard,
		Mouse,

		// common to mobile
		Accelerometer,
		Gyroscope,
		Touch,

		GameController,

		EventTypeCount
	}; // EventType


	// Buttons should be normalized: [0, 1]
	// This allows digital buttons to be either 0 or 1
	// and analog buttons (like Xbox triggers) be analog between 0 and 1.

	// Joystick Axes should be normalized in the range: [-1, 1]


	// EventSubTypes do not need a separate event structure.
	// These provide the specific event for which to extract
	// information from the structure.
	enum EventSubType
	{
		// Window events mainly apply to platforms where the user
		// has control over the application windows (Linux, MacOS X, Windows).

		// A window lost focus.
		WindowLostFocus,

		// A window gained focus.
		WindowGainFocus,

		// A window was resized by the user.
		WindowResized,

		// A window was minimized (i.e. is no longer on screen)
		WindowMinimized,

		// A window has been restored (i.e. window is back on screen)
		WindowRestored,

		// A window was closed by the user.
		WindowClosed,

		MouseButton,
		MouseMoved,
		MouseDelta,
		MouseWheelMoved,

		TouchBegin,
		TouchMoved,
		TouchEnd,

		JoystickButton,
		JoystickAxisMoved,
		JoystickConnected,
		JoystickDisconnected
	}; // EventSubType

	//
	// These are all events used by the kernel. Any new events that a kernel should
	// support must be added here.
	//

	template <EventType type>
	struct Event
	{
		static const EventType event_type = type;
		EventSubType subtype;
		struct platform::window::NativeWindow* window;
	}; // Event

	// call this when the resolution of the window or device has changed
	struct SystemEvent : public Event<System>
	{
		short window_width;
		short window_height;
		short render_width;
		short render_height;

		// TODO: keep track of device orientation
	}; // SystemEvent

	struct KeyboardEvent : public Event<Keyboard>
	{
		uint32_t unicode;
		int key;
		uint16_t modifiers;
		uint16_t is_text;
		bool is_down;
	}; // KeyboardEvent

	struct MouseEvent : public Event<Mouse>
	{
		unsigned int button;

		// absolute mouse values in local (window) coordinates
		int mx;
		int my;

		// delta mouse values
		int dx;
		int dy;

		// < 0 is movement towards the user; > 0 is movement away toward the screen
		// Should be normalized [-1, 1].
		short wheel_direction;
		bool is_down;

		MouseEvent() :
			button(0),
			mx(0),
			my(0),
			dx(0),
			dy(0),
			wheel_direction(0),
			is_down(false)
		{
		}
	}; // MouseEvent

	struct TouchEvent : public Event<Touch>
	{
		int id;

		// coordinates where the touch event began
		int start_x;
		int start_y;

		// current x/y for this touch
		int x;
		int y;
	}; // TouchEvent

	struct GameControllerEvent : public Event<GameController>
	{
		uint8_t gamepad_id;
		uint8_t axis_id;
		uint16_t axis_value;

		float normalized_value() const;
	}; // GameControllerEvent

	void assign_listener_for_eventtype(kernel::EventType type, IKernel* listener);
	IKernel* find_listener_for_eventtype(kernel::EventType type);
} // namespace kernel

namespace gemini
{
	// A general monolithic Kernel event used to dispatch
	// platform-specific events to the application.
	struct KernelEvent
	{
		kernel::EventType type;
		kernel::EventSubType subtype;
		struct platform::window::NativeWindow *window;

		// System
		int16_t window_width;
		int16_t window_height;
		int16_t render_width;
		int16_t render_height;

		// Keyboard
		uint32_t unicode;
		int32_t key;
		uint16_t modifiers;
		uint16_t is_text;
		bool is_down;

		// Mouse
		uint16_t button;

		// absolute mouse values in local (window) coordinates
		int32_t mx;
		int32_t my;

		// delta mouse values
		int32_t dx;
		int32_t dy;

		// < 0 is movement towards the user; > 0 is movement away toward the screen
		// Should be normalized [-1, 1].
		int16_t wheel_direction;

		// GameControllerEvent
		uint8_t gamepad_id;
		uint8_t axis_id;
		uint16_t axis_value;


		KernelEvent();
	};


	// A compact structure after translating from KernelEvent.
	// This is what the application uses to dispatch input.
	// It is intended to be serialized to disk.
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
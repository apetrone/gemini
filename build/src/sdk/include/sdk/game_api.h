// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <stdint.h>
#include <core/mathlib.h>
#include <platform/input.h>

namespace kernel
{
	struct KeyboardEvent;
	struct MouseEvent;
	struct SystemEvent;
	struct GameControllerEvent;
} // namespace kernel

namespace input
{
	class InputState;
} // namespace input

namespace gemini
{
	struct UserCommand;


	struct GameMessage
	{
		enum Type
		{
			KeyboardEvent		= 1, // 1
			// button: keycode
			// params[0]: is_down
			// params[1]: keyboard modifiers

			MouseEvent			= 8,   // 8
			// button: mouse button
			// params[0]: is_down

			MouseMove			= MouseEvent | 16,
			// params[0]: abs mouse x
			// params[1]: abs mouse y

			MouseDelta			= MouseEvent | 32,
			// params[0]: delta mouse x
			// params[1]: delta mouse y

			MouseWheel			= MouseEvent | 64,
			// button: wheel delta
			// params[0]: absolute mouse x
			// params[1]: absolute mouse y
			// params[2]: delta mouse x
			// params[3]: delta mouse y

			GamePadConnected	= 512,
			// params[0]: gamepad_id

			GamePadDisconnected = 1024,
			// params[0]: gamepad_id

			GamePadButton		= 2048,
			// button: gamepad button
			// params[0]: gamepad_id
			// params[1]: is_down

			GamePadAxis			= 4096,
			// params[0]: gamepad_id
			// params[1]: axis_id
			// params[2]: axis_value
		};

		uint32_t type;

		uint32_t button;
		int32_t params[4];
		glm::quat orientation;

		GameMessage() :
			type(0),
			button(0)
		{
			params[0] = params[1] = params[2] = params[3] = 0;
		}
	};

	struct UserCommand
	{
		int sequence;
		//uint32_t buttonflags;
		float angles[2]; // pitch, yaw
		int16_t axes[4];

		UserCommand()
		{
			sequence = 0;
			//buttonflags = 0;
			angles[0] = angles[1] = 0;
			memset(axes, 0, sizeof(int16_t));
		}

		//void set_button(int index, bool is_down)
		//{
		//	if (is_down)
		//	{
		//		buttonflags |= (1 << index);
		//	}
		//	else
		//	{
		//		buttonflags &= ~(1 << index);
		//	}
		//}

		void set_axis(uint32_t axis_id, int16_t value)
		{
			axes[axis_id] = value;
		}
	};

	// Describes the interface exposed to the engine from the game.
	class IGameInterface
	{
	public:
		virtual ~IGameInterface() {};

		// called when the engine connects to the game library
		virtual bool startup() = 0;

		// called just before the engine disconnects from the game library
		virtual void shutdown() = 0;

		// called on level change
		virtual void level_load() = 0;

		// called each tick of the engine
		virtual void server_frame(uint64_t current_ticks, float framedelta_seconds, float step_interval_seconds, float step_alpha) = 0;
		virtual void client_frame(float framedelta_seconds, float step_alpha) = 0;

		// event handling
		virtual void on_event(const kernel::KeyboardEvent& event) = 0;
		virtual void on_event(const kernel::MouseEvent& event) = 0;
		virtual void on_event(const kernel::SystemEvent& event) = 0;
		virtual void on_event(const kernel::GameControllerEvent& event) = 0;
	}; // GameInterface


	class IClientGameInterface
	{
		virtual void run_frame() = 0;
	}; // IClientGameInterface
} // namespace gemini

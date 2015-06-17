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

#include <platform/input.h>

namespace gemini
{
	struct UserCommand;

	
	struct GameMessage
	{
		enum Type
		{
			KeyboardEvent = 1, // 1
			// button: keycode
			// params[0]: is_down
			
			MouseEvent    = 8,   // 8
			// button: mouse button
			// params[0]: is_down

			MouseMove     = MouseEvent | 16,
			// params[0]: abs mouse x
			// params[1]: abs mouse y
			
			MouseDelta	  = MouseEvent | 32,
			// params[0]: rel mouse x
			// params[1]: rel mouse y
			
			MouseWheel    = MouseEvent | 64,
			// params[0]: wheel delta
			
			
			Orientation	  = 512
			// params[0]: quat.x
			// params[1]: quat.y
			// params[2]: quat.z
			// params[3]: quat.w
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
		uint32_t buttonflags;
		float angles[2]; // pitch, yaw
		
		UserCommand()
		{
			sequence = 0;
			buttonflags = 0;
			angles[0] = angles[1] = 0;
		}
		
		void set_button(int index, bool is_down)
		{
			if (is_down)
			{
				buttonflags |= (1 << index);
			}
			else
			{
				buttonflags &= ~(1 << index);
			}
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
		
		// called on the server: process an incoming message
		virtual void server_process_message(const GameMessage& message) = 0;
	}; // GameInterface
	
	
	class IClientGameInterface
	{
		virtual void run_frame() = 0;
	}; // IClientGameInterface
} // namespace gemini
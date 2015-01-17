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

namespace gemini
{

	
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

	namespace physics
	{
		typedef int16_t MovementValue;
		struct MovementCommand
		{
			unsigned int time;
			MovementValue left;
			MovementValue right;
			MovementValue forward;
			MovementValue back;
			
			MovementCommand()
			{
				memset(this, 0, sizeof(MovementCommand));
			}
		};
	}

	class IExperimental
	{
	public:
		virtual ~IExperimental() {}
		
		
		
		virtual void get_player_command(uint8_t index, physics::MovementCommand& command) = 0;
	};
} // namespace gemini
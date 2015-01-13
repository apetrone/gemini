// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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
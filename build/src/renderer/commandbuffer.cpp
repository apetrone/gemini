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
#include "commandbuffer.h"
#include <assert.h>

namespace render2
{
	// ---------------------------------------------------------------------
	// Pass
	// ---------------------------------------------------------------------
	Pass::Pass() :
		target(nullptr),
		clear_color(false),
		clear_depth(false),
		clear_stencil(false),
		depth_test(true),
		cull_mode(CullMode::Backface)
	{
		color(0, 0, 0, 0);
	}

	void Pass::color(float red, float green, float blue, float alpha)
	{
		target_color[0] = red;
		target_color[1] = green;
		target_color[2] = blue;
		target_color[3] = alpha;
	}

	CommandQueue::CommandQueue(gemini::Allocator& _allocator, const Pass& _pass)
		: allocator(_allocator)
		, commands(allocator)
	{
		pass = _pass;
	}

	CommandQueue::~CommandQueue()
	{
	}

	CommandQueue& CommandQueue::operator=(const CommandQueue& other)
	{
		allocator = other.allocator;
		pass = other.pass;

		// For now, shallow copy of commands.
		commands = other.commands;
		return *this;
	}

	void CommandQueue::add_command(const Command& command)
	{
		commands.push_back(command);
	}

	void CommandQueue::reset()
	{
		commands.resize(0);
	}
} // namespace render2

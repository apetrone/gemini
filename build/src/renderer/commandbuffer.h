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
#pragma once

#include <core/typedefs.h>
#include <core/array.h>

namespace render2
{
	enum CommandType
	{
		COMMAND_INVALID,			// invalid command
		COMMAND_SET_VERTEX_BUFFER,	// change vertex buffer
		COMMAND_DRAW,				// draw from vertex buffer
		COMMAND_DRAW_INDEXED,		// draw from index buffer
		COMMAND_PIPELINE,			// set the rendering pipeline
		COMMAND_VIEWPORT,			// set viewport
		COMMAND_TEXTURE,			// set a texture
		COMMAND_STATE
	};

	// It is assumed that front faces are counter-clock wise.
	enum class CullMode
	{
		None,						// don't cull faces
		Frontface,					// cull front faces
		Backface					// cull back faces
	};

	struct Command
	{
		CommandType type;
		void* data[2];
		size_t params[4];

		Command(CommandType command_type = COMMAND_INVALID,
				void* data0 = 0,
				void* data1 = 0,
				size_t param0 = 0,
				size_t param1 = 0,
				size_t param2 = 0,
				size_t param3 = 0);
	};

	// ---------------------------------------------------------------------
	// Pass
	// ---------------------------------------------------------------------
	struct Pass
	{
		Pass() :
			target(nullptr),
			clear_color(false),
			clear_depth(false),
			clear_stencil(false),
			depth_test(true),
			cull_mode(CullMode::Backface)
		{
			color(0, 0, 0, 0);
		}

		void color(float red, float green, float blue, float alpha);

		// color attachments (4)
		// depth attachment
		// stencil attachment

		struct RenderTarget* target;
		float target_color[4];

		// TODO: turn these into flags!
		bool clear_color;
		bool clear_depth;
		bool clear_stencil;
		bool depth_test;

		CullMode cull_mode;
	};

	struct CommandQueue
	{
		Pass pass;
		Array<Command> commands;

		CommandQueue(gemini::Allocator& allocator, const Pass& pass = Pass());
		void add_command(const Command& command);
		void reset();
	};
} // namespace render2

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

//#include "pipeline.h"
//#include "vertexbuffer.h"

#include <core/typedefs.h>

namespace render2
{
	struct Pass;
	
	enum CommandType
	{
		COMMAND_INVALID,			// invalid command
		COMMAND_SET_VERTEX_BUFFER,	// change vertex buffer
		COMMAND_DRAW,				// draw from vertex buffer
		COMMAND_DRAW_INDEXED,		// draw from index buffer
		COMMAND_PIPELINE,			// set the rendering pipeline
		COMMAND_VIEWPORT,			// set viewport
		COMMAND_STATE
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
	
	struct CommandQueue
	{
		Pass* pass;
		size_t total_commands;
		Command commands[32];
		
		CommandQueue(Pass* pass);
		void add_command(const Command& command);
	};
} // namespace render2

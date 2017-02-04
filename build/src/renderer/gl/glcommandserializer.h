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

#include "commandbuffer.h"

#include <renderer/gl/gemgl.h>

#include <core/mem.h>
#include <core/typedefs.h>

namespace render2
{
	class GLCommandSerializer : public CommandSerializer
	{
	public:
		GLCommandSerializer(gemini::Allocator& allocator, CommandQueue& command_queue);

		GLCommandSerializer& operator=(const GLCommandSerializer& other) = delete;

		virtual void vertex_buffer(Buffer* buffer)
		{
			queue.add_command(
							  Command(COMMAND_SET_VERTEX_BUFFER, buffer)
							  );
		}

		virtual void draw(size_t initial_offset, size_t total, size_t instance_index, size_t index_count)
		{
			queue.add_command(
							  Command(COMMAND_DRAW, 0, 0, initial_offset, total, instance_index, index_count)
							  );
		}

		virtual void draw_indexed_primitives(Buffer* index_buffer, size_t total)
		{
			queue.add_command(
							  Command(COMMAND_DRAW_INDEXED, index_buffer, 0, total, 0, 0, 1)
							  );
		}

		virtual void pipeline(Pipeline* pipeline)
		{
			queue.add_command(
							  Command(COMMAND_PIPELINE, pipeline)
							  );
		}

		virtual void viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			queue.add_command(
							  Command(COMMAND_VIEWPORT, 0, 0, x, y, width, height)
							  );
		}

		virtual void texture(Texture* texture, uint32_t index)
		{
			queue.add_command(
							Command(COMMAND_TEXTURE, texture, 0, index, 0)
							  );
		}

		virtual void constant(
			const char* name,
			void* data,
			size_t data_size);
	private:
		CommandQueue& queue;

		gemini::Allocator& allocator;
	}; // GLCommandSerializer
} // namespace render2

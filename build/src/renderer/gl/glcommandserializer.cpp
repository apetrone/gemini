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
#include "glcommandserializer.h"

#include <core/str.h>

namespace render2
{
	GLCommandSerializer::GLCommandSerializer(CommandQueue& command_queue)
		: queue(command_queue)
	{
		allocator = gemini::memory_allocator_linear(gemini::MEMORY_ZONE_RENDERER, buffer_data, CONSTANT_BUFFER_SIZE);
	}

	void GLCommandSerializer::constant(const char* name, void* data, size_t data_size)
	{
		size_t name_length = core::str::len(name) + 1;
		char* buffer = reinterpret_cast<char*>(MEMORY2_ALLOC(allocator, name_length));
		core::str::copy(buffer, name, name_length);
		buffer[name_length - 1] = '\0';

		queue.add_command(
			Command(COMMAND_CONSTANT, buffer, data, data_size, name_length)
		);
	}
} // namespace render2

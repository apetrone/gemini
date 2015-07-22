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
#include "opengl_device.h"
#include "glcommandserializer.h"
#include "commandbuffer.h"

namespace render2
{
	// ---------------------------------------------------------------------
	// render target
	// ---------------------------------------------------------------------
	void OpenGLDevice::activate_render_target(const RenderTarget& rt)
	{
		
	}
	
	void OpenGLDevice::deactivate_render_target(const RenderTarget& rt)
	{
		
	}
	
	RenderTarget* OpenGLDevice::default_render_target()
	{
		return &default_target;
	}

	// ---------------------------------------------------------------------
	// initialization
	// ---------------------------------------------------------------------
	void OpenGLDevice::init(int backbuffer_width, int backbuffer_height)
	{
		default_target.width = backbuffer_width;
		default_target.height = backbuffer_height;
	}


	// ---------------------------------------------------------------------
	// command serializer
	// ---------------------------------------------------------------------
	CommandSerializer* OpenGLDevice::create_serializer(CommandQueue* command_queue)
	{
		static GLCommandSerializer serializer(*command_queue);
		new (&serializer) GLCommandSerializer(*command_queue);
		return &serializer;
	}
	
	void OpenGLDevice::destroy_serializer(CommandSerializer* serializer)
	{
	}
	
	CommandQueue* OpenGLDevice::create_queue(const Pass& render_pass)
	{
		CommandQueue* next_queue = &queue.next();
		next_queue->pass = render_pass;
		return next_queue;
	}
	
	// ---------------------------------------------------------------------
	// command buffers / submission
	// ---------------------------------------------------------------------
	void OpenGLDevice::queue_buffers(CommandQueue* queue_list, size_t total_queues)
	{
		for (size_t index = 0; index < total_queues; ++index)
		{
			CommandQueue* q = &queue_list[index];
			queued_buffers.push_back(q);
		}
	}
	
	void OpenGLDevice::backbuffer_resized(int backbuffer_width, int backbuffer_height)
	{
		default_target.width = backbuffer_width;
		default_target.height = backbuffer_height;
	}
} // namespace render2

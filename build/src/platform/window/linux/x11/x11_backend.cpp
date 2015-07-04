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
#include "platform.h"
#include "x11_backend.h"

namespace platform
{
	NativeWindow* window_create(const WindowParameters& window_parameters)
	{
		return nullptr;
	}

	void window_destroy(NativeWindow* window)
	{
	}
	
	void window_begin_rendering(NativeWindow* window)
	{	
	}
	
	void window_end_rendering(NativeWindow* window)
	{
	}

	void window_process_events()
	{
	}

	void window_size(NativeWindow* window, int& width, int& height)
	{
	}
	
	void window_render_size(NativeWindow* window, int& width, int& height)
	{
	}
	
	size_t window_screen_count()
	{
		return 0;
	}

	void window_screen_rect(size_t screen_index, int& x, int& y, int& width, int& height)
	{
	}
	
	void window_focus(NativeWindow* window)
	{
	}
	
	void window_show_cursor(bool enable)
	{
	}
} // namespace platform

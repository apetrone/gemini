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

#include <stdint.h> // for types

namespace kernel
{
	struct Parameters;
}

namespace platform
{
	struct WindowParameters
	{
		// in windowed modes, this is the target display the window
		// will be transferred to
		uint32_t target_display;
		
		// dimensions of the actual window in pixels
		uint32_t window_width;
		uint32_t window_height;
		
		// dimensions of the rendering area in pixels
		uint32_t render_width;
		uint32_t render_height;
		
		// need to take this into account when calculating screen coordinates
		uint32_t titlebar_height;

		// utf8-encoded window title
		const char* window_title;
		
		// set to true to create a fullscreen window
		bool enable_fullscreen;
				
		WindowParameters() :
			target_display(0),
			window_width(1),
			window_height(1),
			render_width(0),
			render_height(1),
			titlebar_height(0),
			window_title(0),
			enable_fullscreen(false)
		{
		}
		
		virtual ~WindowParameters();
	};
	
	struct NativeWindow : public WindowParameters
	{
		virtual ~NativeWindow();
	};
	
	struct IWindowLibrary
	{
	public:
		virtual ~IWindowLibrary();
		
		virtual void startup(kernel::Parameters& parameters) = 0;
		virtual void shutdown() = 0;
		virtual NativeWindow* create_window(const WindowParameters& parameters) = 0;
		virtual void destroy_window(NativeWindow* window) = 0;
		virtual void process_events() = 0;
		virtual void activate_window(NativeWindow* window) = 0;
		virtual void swap_buffers(NativeWindow* window) = 0;
		virtual void focus_window(NativeWindow* window) = 0;
		
		// cursor control
		virtual void capture_mouse(bool capture) = 0;
		virtual void warp_mouse(int x, int y) = 0;
		virtual void get_mouse(int& x, int& y) = 0;
		virtual void show_mouse(bool show) = 0;
	};
	
	IWindowLibrary* create_window_library();
	void destroy_window_library();
	
} // namespace platform

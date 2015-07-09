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
	// ---------------------------------------------------------------------
	// window system
	// ---------------------------------------------------------------------
	namespace window
	{
		enum RenderingBackend
		{
			// choose the best default for this platform
			RenderingBackend_Default,
			
			// OpenGL and variants
			RenderingBackend_OpenGL,
			RenderingBackend_OpenGLES2,
			RenderingBackend_OpenGLES3
		};

		struct Frame
		{
			uint32_t x;
			uint32_t y;
			uint32_t width;
			uint32_t height;
			
			Frame() :
				x(0),
				y(0),
				width(0),
				height(0)
			{
			}
		};
		
		struct WindowDimensions
		{
			uint32_t x;
			uint32_t y;
			
			// dimensions of the actual window in pixels
			uint32_t width;
			uint32_t height;
			
			// dimensions of the rendering area in pixels
			uint32_t render_width;
			uint32_t render_height;
			
			WindowDimensions() :
				x(0),
				y(0),
				width(1),
				height(1),
				render_width(0),
				render_height(1)
			{
			}
		};
		
		struct Parameters
		{
			WindowDimensions window;
			
			// in windowed modes, this is the target display the window
			// will be transferred to
			uint32_t target_display;
			
			// need to take this into account when calculating screen coordinates
			uint32_t titlebar_height;
			
			// utf8-encoded window title
			const char* window_title;
			
			// depth size requested for this window in bits
			uint8_t depth_size;
			
			// set to true to create a fullscreen window
			bool enable_fullscreen;
			
			// allow the window to be resized
			bool enable_resize;
			
			// wait for vertical sync
			bool enable_vsync;
			
			Parameters() :
				target_display(0),
				titlebar_height(0),
				window_title(0),
				enable_fullscreen(false),
				enable_resize(true),
				enable_vsync(true)
			{
			}
			
			virtual ~Parameters();
		};
		
		struct NativeWindow
		{
			virtual ~NativeWindow();
			
			NativeWindow(const WindowDimensions& window_dimensions) :
				dimensions(window_dimensions),
				graphics_data(0)
			{
			}
			
			/// @brief returns this platform's native window handle
			virtual void* get_native_handle() const = 0;
			
			WindowDimensions dimensions;
			
			// data used by the graphics provider on this system
			void* graphics_data;
		};
		
		class input_provider
		{
		public:
			virtual ~input_provider();
			
			// capture the mouse
			virtual void capture_mouse(bool capture) = 0;
			
			// warp the mouse to a position
			virtual void warp_mouse(int x, int y) = 0;
			
			// get the current mouse position
			virtual void get_mouse(int& x, int& y) = 0;
			
			// toggle mouse visibility
			virtual void show_mouse(bool show) = 0;
		};
		
		/// @brief startup the window backend
		/// @returns Result Success or Failure with a message
		LIBRARY_EXPORT Result startup(RenderingBackend backend);

		/// @brief safely shutdown the window backend
		LIBRARY_EXPORT void shutdown();
		
		/// @brief Call this once per frame in your application
		/// To allow the system backend to handle and dispatch window events
		LIBRARY_EXPORT void dispatch_events();

		LIBRARY_EXPORT NativeWindow* create(const Parameters& window_parameters);
		LIBRARY_EXPORT void destroy(NativeWindow* window);
		
		// activate this window for rendering
		LIBRARY_EXPORT void begin_rendering(NativeWindow* window);
		
		// post frame on this window
		LIBRARY_EXPORT void end_rendering(NativeWindow* window);
		
		// return the window size in pixels
		LIBRARY_EXPORT Frame get_frame(NativeWindow* window);
		
		// return the renderable window surface in pixels
		LIBRARY_EXPORT Frame get_render_frame(NativeWindow* window);
		
		// total number of screens detected on this system
		LIBRARY_EXPORT size_t screen_count();
		
		/// @brief get the specified screen's rect (origin, width, and height) in pixels
		LIBRARY_EXPORT Frame screen_frame(size_t screen_index);
		
		// bring window to focus
		LIBRARY_EXPORT void focus(NativeWindow* window);
		
		// show or hide the mouse cursor
		LIBRARY_EXPORT void show_cursor(bool enable);
	} // namespace window
} // namespace platform

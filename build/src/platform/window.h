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
			// origin is in the upper left
			// lower right is (x+width, y+height)
			float x;
			float y;
			float width;
			float height;

			Frame() :
				x(0),
				y(0),
				width(0),
				height(0)
			{
			}
		};

		struct Parameters
		{
			// This is the window's frame dimensions used to
			// create and position the new window.
			// It is in screen coordinates -- so if you want the window on
			// another display, you must calculate appropriately.
			Frame frame;

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
			NativeWindow() :
				graphics_data(nullptr)
			{
			}

			virtual ~NativeWindow();

			/// @brief returns this platform's native window handle
			virtual void* get_native_handle() = 0;

			/// @brief Notification from the graphics provider
			/// when the native visual id has been changed
			virtual void update_visual(int visual) {}

			/// @brief An update to this window's dimensions happened
			virtual void update_size(int width, int height) {}

			// data used by the graphics provider on this system
			void* graphics_data;
		};

		class InputProvider
		{
		public:
			virtual ~InputProvider();

			// capture the mouse
			virtual void capture_mouse(bool capture) = 0;

			// warp the mouse to a position
			virtual void warp_mouse(int x, int y) = 0;

			// get the current mouse position
			virtual void get_mouse(int& x, int& y) = 0;

			// toggle mouse visibility
			virtual void show_mouse(bool show) = 0;
		};


		// given a screen and target window dimensions, return a centered frame
		LIBRARY_EXPORT Frame centered_window_frame(size_t screen_index, size_t width, size_t height);

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
		LIBRARY_EXPORT void activate_context(NativeWindow* window);

		// deactivate this window for rendering
		LIBRARY_EXPORT void deactivate_context(NativeWindow* window);

		// swap buffers on this window
		LIBRARY_EXPORT void swap_buffers(NativeWindow* window);

		// return the window size in screen coordinates
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

		// set the cursor position in screen coordinates
		LIBRARY_EXPORT void set_cursor(float x, float y);

		// get the cursor position in screen coordinates
		LIBRARY_EXPORT void get_cursor(float& x, float& y);
	} // namespace window
} // namespace platform

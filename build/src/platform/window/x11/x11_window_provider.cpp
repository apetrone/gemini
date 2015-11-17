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
#include "platform_internal.h"
#include "linux_backend.h"
#include "x11_window_provider.h"


#include <X11/keysym.h>
#include <X11/extensions/Xrandr.h>

// requires: libxinerama-dev (ubuntu)
// see if we can replace this with XRandR
#include <X11/extensions/Xinerama.h>

namespace platform
{
	namespace window
	{
		X11WindowProvider::X11WindowProvider() :
			display(0)
		{
		}

		X11WindowProvider::~X11WindowProvider()
		{
		}

		Result X11WindowProvider::startup()
		{
			// try to open the display
			display = XOpenDisplay(0);

			if (!display)
			{
				return Result::failure("Unable to open display 0");
			}
			return Result::success();
		}

		void X11WindowProvider::shutdown()
		{
			if (display)
			{
				XCloseDisplay(display);
			}
		}

		NativeWindow* X11WindowProvider::create(const Parameters& parameters, void* native_visual)
		{
			X11Window* window = MEMORY_NEW(X11Window, get_platform_allocator());
			Visual* visual = static_cast<Visual*>(native_visual);
			assert(visual != nullptr);

			XSetWindowAttributes window_attributes;

			window_attributes.border_pixel = 0;
			window_attributes.background_pixel = 0;

			window_attributes.event_mask = StructureNotifyMask;
			window_attributes.colormap = XCreateColormap(
				display,
				RootWindow(display, DefaultScreen(display)),
				visual,
				AllocNone);

			window->native_window = XCreateWindow(
				display,
				RootWindow(display, DefaultScreen(display)),
				parameters.frame.x,
				parameters.frame.y,
				parameters.frame.width,
				parameters.frame.height,
				0,
				CopyFromParent,
				InputOutput,
				visual,
				CWColormap | CWEventMask,
				&window_attributes);

			XMapWindow(display, window->native_window);

			// manually flush the output buffer
			// happens as part of: XPending, XNextEvent, and XWindowEvent
			XFlush(display);

			return window;
		}

		void X11WindowProvider::destroy(NativeWindow* window)
		{
			void* native_handle = window->get_native_handle();
			Window* window_handle = static_cast<Window*>(native_handle);
			XDestroyWindow(display, *window_handle);
			MEMORY_DELETE(window, get_platform_allocator());
		}

		Frame X11WindowProvider::get_frame(NativeWindow* window) const
		{
			Frame frame;

			Window root_window;
			int x;
			int y;
			unsigned int width;
			unsigned int height;
			unsigned int border_width;
			unsigned int depth;

			void* native_handle = window->get_native_handle();
			Window* window_handle = static_cast<Window*>(native_handle);

			XGetGeometry(
				display,
				*window_handle,
				&root_window,
				&x, &y,
				&width, &height,
				&border_width, &depth);

			frame.x = static_cast<float>(x);
			frame.y = static_cast<float>(y);
			frame.width = static_cast<float>(width);
			frame.height = static_cast<float>(height);

			return frame;
		}


		Frame X11WindowProvider::get_render_frame(NativeWindow* window) const
		{
			Frame frame;
			return frame;
		}

		size_t X11WindowProvider::get_screen_count() const
		{
			int dummy[2];
			if (XineramaQueryExtension(display, &dummy[0], &dummy[1]))
			{
				if (XineramaIsActive(display))
				{
					int total_screens = 0;
					XineramaScreenInfo* screens = XineramaQueryScreens(display, &total_screens);
					XFree(screens);
					return total_screens;
				}
			}
			return ScreenCount(display);
		}

		Frame X11WindowProvider::get_screen_frame(size_t screen_index) const
		{
			Frame frame;
			int dummy[2];
			if (XineramaQueryExtension(display, &dummy[0], &dummy[1]))
			{
				if (XineramaIsActive(display))
				{
					int total_screens = 0;
					XineramaScreenInfo* screens = XineramaQueryScreens(display, &total_screens);
					if (total_screens > 0)
					{
						XineramaScreenInfo& screen = screens[screen_index];
						frame.x = screen.x_org;
						frame.y = screen.y_org;
						frame.width = screen.width;
						frame.height = screen.height;
					}

					XFree(screens);
				}
			}

			return frame;
		}

		Display* X11WindowProvider::get_display() const
		{
			return display;
		}
	} // namespace window
} // namespace platform

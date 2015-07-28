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
#include "x11_graphics_provider.h"
#include "window/x11/x11_window_provider.h"

#include <GL/glx.h>

#include <assert.h>

#define PLATFORM_DEBUG_X11 0

namespace platform
{
	namespace window
	{
		struct X11Data
		{
		};

		X11GraphicsProvider::X11GraphicsProvider()
		{
		}

		Result X11GraphicsProvider::startup(WindowProvider* wp)
		{
			window_provider = static_cast<X11WindowProvider*>(wp);
			assert(window_provider != nullptr);

			// see if we can get the major/minor version of GLX
			int major;
			int minor;

			Display* display = window_provider->get_display();
			Bool result = glXQueryVersion(display, &major, &minor);
			if (!result)
			{
				PLATFORM_LOG(LogMessageType::Warning, "Error fetching GLX version!\n");
			}
			else
			{
				PLATFORM_LOG(LogMessageType::Info, "GLX version: %d.%d\n", major, minor);
			}

			return Result::success();
		}

		void X11GraphicsProvider::shutdown(WindowProvider* window_provider)
		{
		}

		void X11GraphicsProvider::create_context(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::destroy_context(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::attach_context(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::detach_context(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::create_surface(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::destroy_surface(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::swap_buffers(NativeWindow* window)
		{
		}

		void* X11GraphicsProvider::get_symbol(const char* symbol_name)
		{
			return nullptr;
		}

		size_t X11GraphicsProvider::get_graphics_data_size() const
		{
			return sizeof(int);
		}

		int X11GraphicsProvider::choose_pixel_format(const Parameters& parameters)
		{
			int attributes[] = {
				GLX_RGBA,
				GLX_DOUBLEBUFFER,
				GLX_STEREO, 0,
				GLX_RED_SIZE, 8,
				GLX_GREEN_SIZE, 8,
				GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, 0,
				GLX_DEPTH_SIZE, 0,
				GLX_STENCIL_SIZE, 0,
				0
			};

			XVisualInfo* visual = glXChooseVisual(
				window_provider->get_display(),
				DefaultScreen(window_provider->get_display()),
				attributes
				);

			assert(visual);

			// return the visual as the "pixel format"
			return visual->visualid;
		}
	} // namespace window

} // namespace platform

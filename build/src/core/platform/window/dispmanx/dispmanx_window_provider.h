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

#include <core/config.h>
#include "window_provider.h"

#if defined(PLATFORM_RASPBERRYPI)
	#include <bcm_host.h> // for DISPMANX_* types
#endif

// needed for EGL_DISPMANX_NATIVE_WINDOW_T
#if defined(PLATFORM_EGL_SUPPORT)
	#include <EGL/egl.h>
#else
	#error PLATFORM_EGL_SUPPORT not defined. Raspberry Pi build requires EGL support!
#endif

namespace platform
{
	namespace window
	{
		struct DispManXWindow : public NativeWindow
		{
			virtual void* get_native_handle() override
			{
				return &native_window;
			}

			virtual void update_size(int width, int height)
			{
				this->width = width;
				this->height = height;
			}

#if defined(PLATFORM_RASPBERRYPI)
			EGL_DISPMANX_WINDOW_T native_window;
#endif
			int32_t width;
			int32_t height;
		}; // struct DispManXWindow

		class DispManXWindowProvider : public WindowProvider
		{
		public:
			virtual Result startup();
			virtual void shutdown();
			virtual NativeWindow* create(const Parameters& parameters, void* native_visual);
			virtual void destroy(NativeWindow* window, DestroyWindowBehavior behavior) override;
			virtual Frame get_frame(NativeWindow* window) const;
			virtual Frame get_render_frame(NativeWindow* window) const;
			virtual size_t get_screen_count() const;
			virtual Frame get_screen_frame(size_t screen_index) const;
			virtual void dispatch_events() override;

		private:
			DISPMANX_DISPLAY_HANDLE_T dispman_display;
			DISPMANX_UPDATE_HANDLE_T dispman_update;
			DISPMANX_ELEMENT_HANDLE_T dispman_element;

			DispManXWindow main_window;
		}; // class DispManXWindowProvider
	} // namespace window
} // namespace platform

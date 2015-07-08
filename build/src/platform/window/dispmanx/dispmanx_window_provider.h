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
#include "linux_backend.h"

#if defined(PLATFORM_RASPBERRYPI)
	#include <bcm_host.h> // for DISPMANX_* types
#endif

namespace platform
{
	namespace linux
	{
		class DispManXWindowProvider : public WindowProvider
		{
		public:
			DispManXWindowProvider();
			virtual ~DispManXWindowProvider();

			virtual Result startup();
			virtual void shutdown();
			virtual NativeWindow* create(const WindowParameters& parameters);
			virtual void destroy(NativeWindow* window);
			virtual Frame get_window_rect(NativeWindow* window) const;
			virtual Frame get_window_render_rect(NativeWindow* window) const;
			virtual size_t get_screen_count() const;
			virtual Frame get_screen_rect(size_t screen_index) const;

		private:
			uint32_t display_width;
			uint32_t display_height;

			DISPMANX_DISPLAY_HANDLE_T dispman_display;
			DISPMANX_UPDATE_HANDLE_T dispman_update;
			DISPMANX_ELEMENT_HANDLE_T dispman_element;
		}; // class DispManXWindowProvider
	} // namespace linux
} // namespace platform

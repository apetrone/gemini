// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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

#if defined(PLATFORM_RASPBERRYPI)
	#include <bcm_host.h>

	// PLATFORM_SUPPORT_EGL and PLATFORM_SUPPORT_OPENGLES must be defined
	#if !(defined(PLATFORM_EGL_SUPPORT) || !defined(PLATFORM_OPENGLES_SUPPORT))
		#error RaspberryPi requires EGL and OpenGL ES support!
	#endif
#endif


#if defined(PLATFORM_EGL_SUPPORT)
	#include "../../window/linux/egl/egl_backend.h"
#endif

namespace platform
{

	Result os_startup()
	{
		linux_window_backend_startup();

		return Result(Result::Success);
	}
	
	void os_shutdown()
	{
		linux_window_backend_shutdown();
	}
	
	int os_run_application(int argc, const char** argv)
	{
		return 0;
	}


	void linux_window_backend_startup()
	{
#if defined(PLATFORM_RASPBERRYPI)
		fprintf(stdout, "RaspberryPi!\n");

		// this must be called before we can issue any GPU commands
		bcm_host_init();


		// get the display width/height
		uint32_t display_width;
		uint32_t display_height;

		int success = graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
		if (success != 0)
		{
			fprintf(stderr, "Failed to get display size!\n");
			return;
		}

		fprintf(stdout, "display resolution: %i x %i\n", display_width, display_height);

		// EGL_DISPMANX_WINDOW_T native_window;
		DISPMANX_DISPLAY_HANDLE_T dispman_display;
		DISPMANX_UPDATE_HANDLE_T dispman_update;		
		DISPMANX_ELEMENT_HANDLE_T dispman_element;

		VC_RECT_T dst_rect;
		VC_RECT_T src_rect;
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.width = display_width;
		dst_rect.height = display_height;

		src_rect.x = 0;
		src_rect.y = 0;
		src_rect.width = display_width << 16;
		src_rect.height = display_height << 16;

		dispman_display = vc_dispmanx_display_open(0);
		dispman_update = vc_dispmanx_update_start(0);

		dispman_element = vc_dispmanx_element_add(dispman_update,
			dispman_display, 0, &dst_rect, 0, &src_rect, 
			DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0 /* clamp */,
			(DISPMANX_TRANSFORM_T)0 /* transform*/
		);

		// native_window.element = dispman_element;
		// native_window.width = display_width;
		// native_window.height = display_height;
		vc_dispmanx_update_submit_sync(dispman_update);
#endif



#if defined(PLATFORM_EGL_SUPPORT)
		egl_backend_startup();
#endif
	}

	void linux_window_backend_shutdown()
	{
#if defined(PLATFORM_EGL_SUPPORT)
		egl_backend_shutdown();
#endif
	}
} // namespace platform
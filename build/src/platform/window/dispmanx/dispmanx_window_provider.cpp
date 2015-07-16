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
#include "dispmanx_window_provider.h"

#if defined(PLATFORM_RASPBERRYPI)
	#include <bcm_host.h> // for graphics_get_display_size

	// PLATFORM_SUPPORT_EGL and PLATFORM_SUPPORT_OPENGLES must be defined
	#if !(defined(PLATFORM_EGL_SUPPORT) || !defined(PLATFORM_GLES2_SUPPORT))
		#error RaspberryPi requires EGL and OpenGL ES support!
	#endif

	// needed for EGL_DISPMANX_NATIVE_WINDOW_T
	#if defined(PLATFORM_EGL_SUPPORT)
		#include <EGL/egl.h>
	#else
		#error PLATFORM_EGL_SUPPORT not defined. Raspberry Pi build requires EGL support!
	#endif
#endif

namespace platform
{
	namespace window
	{
		struct DispManXWindow : public NativeWindow
		{
			DispManXWindow(const WindowDimensions& window_dimensions) :
				NativeWindow(window_dimensions)
			{				
			}

#if defined(PLATFORM_RASPBERRYPI)
			EGL_DISPMANX_WINDOW_T native_window;
#endif		

			virtual void* get_native_handle() const override
			{
				return (void*)&native_window;
			}
		}; // struct DispManXWindow


		DispManXWindowProvider::DispManXWindowProvider() :
			display_width(0),
			display_height(0)
		{
		}

		DispManXWindowProvider::~DispManXWindowProvider()
		{
		}


		Result DispManXWindowProvider::startup()
		{
#if defined(PLATFORM_RASPBERRYPI)
			dispman_display = vc_dispmanx_display_open(0);

			// get the display width/height
			DISPMANX_MODEINFO_T info;
			int result = vc_dispmanx_display_get_info(dispman_display, &info);
			if (result != 0)
			{
				return Result(Result::Failure, "Failed to get display size!");
			}

			display_width = info.width;
			display_height = info.height;

			// next, setup the dispman element which we need for the
			// window surface.
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

			// disable the alpha layer; fully opaque rendering
			VC_DISPMANX_ALPHA_T alpha;
			alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
			alpha.opacity = 255;
			alpha.mask = 0;

			dispman_update = vc_dispmanx_update_start(0);
			dispman_element = vc_dispmanx_element_add(dispman_update,
				dispman_display, 0, &dst_rect, 0, &src_rect, 
				DISPMANX_PROTECTION_NONE, &alpha, 0 /* clamp */,
				(DISPMANX_TRANSFORM_T)0 /* transform*/
			);

			vc_dispmanx_update_submit_sync(dispman_update);
#endif

			return Result(Result::Success);
		}

		void DispManXWindowProvider::shutdown()
		{
			int result = 0;
			result = vc_dispmanx_element_remove(dispman_update, dispman_element);
			assert(result == 0);

			vc_dispmanx_update_submit_sync(dispman_update);
			assert(result == 0);

			vc_dispmanx_display_close(dispman_display);
			assert(result == 0);
		}

		NativeWindow* DispManXWindowProvider::create(const Parameters& parameters)
		{
			DispManXWindow* window = MEMORY_NEW(DispManXWindow, platform::get_platform_allocator())(parameters.window);
			window->native_window.width = display_width;
			window->native_window.height = display_height;
			window->native_window.element = dispman_element;
			return window;
		}

		void DispManXWindowProvider::destroy(NativeWindow* window)
		{
			DispManXWindow* pointer = static_cast<DispManXWindow*>(window);
			MEMORY_DELETE(pointer, platform::get_platform_allocator());
		}

		Frame DispManXWindowProvider::get_frame(NativeWindow* window) const
		{
			Frame frame;
#if defined(PLATFORM_RASPBERRYPI)
			frame.width = display_width;
			frame.height = display_height;
#endif			
			return frame;
		}

		Frame DispManXWindowProvider::get_render_frame(NativeWindow* window) const
		{
			Frame frame;
#if defined(PLATFORM_RASPBERRYPI)
			frame.width = display_width;
			frame.height = display_height;
#endif			
			return frame;
		}

		size_t DispManXWindowProvider::get_screen_count() const
		{
#if defined(PLATFORM_RASPBERRYPI)
			return 1;
#else
			#error Not implemented on this platform!
#endif
			return 0;
		}

		Frame DispManXWindowProvider::get_screen_frame(size_t screen_index) const
		{
			Frame frame;
#if defined(PLATFORM_RASPBERRYPI)
			assert(screen_index == 0);
			frame.width = display_width;
			frame.height = display_height;
#endif
			return frame;
		}		
	} // namespace window
} // namespace platform
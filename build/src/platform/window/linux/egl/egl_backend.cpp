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
#include "egl_backend.h"

#include <EGL/egl.h>
#include <assert.h>

namespace platform
{
	struct egl_window : public NativeWindow
	{
		egl_window(const WindowDimensions& window_dimensions) : 
			NativeWindow(window_dimensions)
		{
		}

		EGLContext context;
		EGLSurface surface;
	};

	static EGLDisplay _default_display = EGL_NO_DISPLAY;

	Result egl_backend_startup()
	{
		assert(_default_display == EGL_NO_DISPLAY);

		_default_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		assert(_default_display != EGL_NO_DISPLAY);

		int major = 0;
		int minor = 0;
		EGLBoolean initialized = eglInitialize(_default_display, &major, &minor);
		assert(initialized != EGL_FALSE);

		fprintf(stderr, "EGL version is %i.%i\n", major, minor);

		// eglBindAPI and EGL version required
		// 1.2+: EGL_OPENGL_ES_API and EGL_OPENVG_API
		// 1.4+: EGL_OPENGL_API
#if defined(PLATFORM_OPENGLES_SUPPORT)
		assert(major >= 1 && minor >= 2);
		eglBindAPI(EGL_OPENGL_ES_API);
#elif defined(PLATFORM_OPENGL_SUPPORT)
		assert(major >= 1 && minor >= 4);
		eglBindAPI(EGL_OPENGL_API);
#else
		#error No compatible graphics provider
#endif

		return Result (Result::Success);
	}

	void egl_backend_shutdown()
	{
		assert(_default_display != EGL_NO_DISPLAY);

		eglMakeCurrent(_default_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglTerminate(_default_display);
		_default_display = EGL_NO_DISPLAY;
	}


	NativeWindow* window_create(const WindowParameters& window_parameters)
	{
		// prepare egl attributes
		EGLint attribs[] = {
#if defined(PLATFORM_OPENGLES_SUPPORT)
			// OpenGL ES 1.0/1.1
			// EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
			
			// OpenGL ES 2.0
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#endif

			EGL_CONFIG_CAVEAT, EGL_DONT_CARE,

			// depth size in bits
			EGL_DEPTH_SIZE, window_parameters.depth_size,

			// color buffer size (can specify up to 32)
			EGL_BUFFER_SIZE, 24,

			EGL_NONE
		};


		// EGLint context_attribs[] = {
		// 	EGL_CONTEXT_CLIENT_VERSION, 2,
		// 	EGL_NONE,
		// };

		EGLDisplay display = _default_display;

		EGLConfig config;
		EGLint total_configs;
		EGLBoolean result = EGL_FALSE;

		result = eglChooseConfig(display, attribs, &config, 1, &total_configs);
		assert(result != EGL_FALSE);

		// create an egl window to store more data
		egl_window* window = MEMORY_NEW(egl_window, get_platform_allocator())(window_parameters.window);

		// create the EGL context
		window->context = eglCreateContext(display, config, EGL_NO_CONTEXT, 0);
		assert(window->context != EGL_NO_CONTEXT);


		return window;
	}

	void window_destroy(NativeWindow* window)
	{
		egl_window* eglw = static_cast<egl_window*>(window);
		MEMORY_DELETE(eglw, get_platform_allocator());
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

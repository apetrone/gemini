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
#include "egl_graphics_provider.h"

#include <assert.h>

namespace platform
{
	namespace window
	{
		static EGLint egl_check_error(const char* message)
		{
			EGLint error = eglGetError();
			if (error != EGL_SUCCESS)
			{
				fprintf(stderr, "ERROR on '%s' eglGetError returned %i\n", message, error);
			}

			return error;
		}

		#define EGL_CHECK_ERROR(message)
		// #define EGL_CHECK_ERROR(message) egl_check_error(message)		

		struct EGLData
		{
			EGLint visual;
			EGLSurface surface;
			EGLContext context;
		};

		EGLGraphicsProvider::EGLGraphicsProvider() :
			display(EGL_NO_DISPLAY)
		{
		}

		Result EGLGraphicsProvider::startup()
		{
			assert(display == EGL_NO_DISPLAY);

			display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
			assert(display != EGL_NO_DISPLAY);

			int major = 0;
			int minor = 0;
			EGLBoolean initialized = eglInitialize(display, &major, &minor);
			assert(initialized != EGL_FALSE);

			fprintf(stderr, "EGL version is %i.%i\n", major, minor);

			EGLBoolean bind_result = EGL_FALSE;

			{
				const char* eglstring;
				eglstring = (const char*) eglQueryString(display, EGL_VENDOR);
				fprintf(stdout, "EGL_VENDOR: %s\n", eglstring);

				eglstring = (const char*) eglQueryString(display, EGL_VERSION);
				fprintf(stdout, "EGL_VERSION: %s\n", eglstring);

				eglstring = (const char*) eglQueryString(display, EGL_CLIENT_APIS);
				fprintf(stdout, "EGL_CLIENT_APIS: %s\n", eglstring);

				eglstring = (const char*) eglQueryString(display, EGL_EXTENSIONS);
				fprintf(stdout, "EGL_EXTENSIONS: %s\n", eglstring);
			}

			// eglBindAPI and EGL version required
			// 1.2+: EGL_OPENGL_ES_API and EGL_OPENVG_API
			// 1.4+: EGL_OPENGL_API
#if defined(PLATFORM_OPENGLES_SUPPORT)
			assert(major >= 1 && minor >= 2);
			bind_result = eglBindAPI(EGL_OPENGL_ES_API);
#elif defined(PLATFORM_OPENGL_SUPPORT)
			assert(major >= 1 && minor >= 4);
			bind_result = eglBindAPI(EGL_OPENGL_API);
#else
			#error No compatible graphics provider
#endif
			// eglBindAPI failed!
			assert(bind_result != EGL_FALSE);

			return Result(Result::Success);
		}

		void EGLGraphicsProvider::shutdown()
		{
			assert(display != EGL_NO_DISPLAY);

			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglTerminate(display);

			display = EGL_NO_DISPLAY;
		}

		void EGLGraphicsProvider::create_context(NativeWindow* window)
		{
			EGLData* window_data = reinterpret_cast<EGLData*>(window->graphics_data);
			assert(window_data);

			memset(window_data, 0, sizeof(EGLData));
			window_data->context = EGL_NO_CONTEXT;
			window_data->surface = EGL_NO_SURFACE;

			// prepare egl attributes
			EGLint attribs[] = {
#if defined(PLATFORM_OPENGLES_SUPPORT)
				// OpenGL ES 1.0/1.1
				// EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
				// EGL_CONFORMANT, EGL_OPENGL_ES_BIT,
			
				// OpenGL ES 2.0
				EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
				EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
#endif


				EGL_CONFIG_CAVEAT, EGL_NONE,

				// depth size in bits
				EGL_DEPTH_SIZE, 16,

				// color buffer size (can specify up to 32)
				EGL_BUFFER_SIZE, 24,

				EGL_NONE
			};

			EGLConfig config;
			EGLint total_configs = 0;
			EGLBoolean result = EGL_FALSE;

			// choose one config based on the attributes
			result = eglChooseConfig(display, attribs, &config, 1, &total_configs);
			assert(result != EGL_FALSE);

			// cache the visual id
			// needed by X11
			result = eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &window_data->visual);
			assert(result != EGL_FALSE);

			// create the EGL context
			EGLint context_attribs[] = {
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE,
			};

			EGLContext share_context = EGL_NO_CONTEXT;
			window_data->context = eglCreateContext(display, config, share_context, context_attribs);
			assert(window_data->context != EGL_NO_CONTEXT);
			EGL_CHECK_ERROR("eglCreateContext");

			window_data->surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)window->get_native_handle(), 0);
			EGL_CHECK_ERROR("eglCreateWindowSurface");
			if (window_data->surface == EGL_NO_SURFACE)
			{
				fprintf(stderr, "eglCreateWindowSurface failed!\n");
				return;
			}
		}

		void EGLGraphicsProvider::destroy_context(NativeWindow* window)
		{
			EGLData* window_data = reinterpret_cast<EGLData*>(window->graphics_data);
			
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			EGL_CHECK_ERROR("eglMakeCurrent");

			EGLBoolean result = EGL_FALSE;
			result = eglDestroySurface(display, window_data->surface);
			assert(result != EGL_FALSE);

			result = eglDestroyContext(display, window_data->context);
			assert(result != EGL_FALSE);
		}

		void EGLGraphicsProvider::activate_context(NativeWindow* window)
		{
			EGLData* window_data = reinterpret_cast<EGLData*>(window->graphics_data);
			EGLBoolean result = eglMakeCurrent(display, window_data->surface, window_data->surface, window_data->context);
			EGL_CHECK_ERROR("eglMakeCurrent");
			if (result == EGL_FALSE)
			{
				fprintf(stderr, "eglMakeCurrent failed to activate context\n");
			}
		}

		void EGLGraphicsProvider::swap_buffers(NativeWindow* window)
		{
			EGLData* window_data = reinterpret_cast<EGLData*>(window->graphics_data);
			eglSwapBuffers(display, window_data->surface);
			EGL_CHECK_ERROR("eglSwapBuffers");
		}

		void* EGLGraphicsProvider::get_symbol(const char* symbol_name)
		{
			return 0;
		}

		size_t EGLGraphicsProvider::get_graphics_data_size() const
		{
			return sizeof(EGLData);
		}
	} // namespace window

} // namespace platform

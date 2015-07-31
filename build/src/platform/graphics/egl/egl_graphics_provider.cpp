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

#define PLATFORM_DEBUG_EGL 0

#if PLATFORM_DEBUG_EGL
	#define EGL_CHECK_ERROR(message) egl_check_error(message)
#else
	#define EGL_CHECK_ERROR(message)
#endif

namespace platform
{
	namespace window
	{
#if PLATFORM_DEBUG_EGL
		EGLint egl_check_error(const char* message)
		{
			EGLint error = eglGetError();
			if (error != EGL_SUCCESS)
			{
				PLATFORM_LOG(LogMessageType::Error, "ERROR on '%s' eglGetError returned %i\n", message, error);
			}

			return error;
		}

		#define PRINT_ATTRIB(attrib)\
			result = eglGetConfigAttrib(display, config, attrib, &value);\
			assert(result == EGL_TRUE);\
			PLATFORM_LOG(LogMessageType::Info, "%s: %i\n", #attrib, value)

		void egl_print_config(EGLDisplay display, EGLConfig config)
		{
			EGLBoolean result = EGL_FALSE;
			EGLint value;

			PRINT_ATTRIB(EGL_ALPHA_SIZE);
			PRINT_ATTRIB(EGL_ALPHA_MASK_SIZE);
			PRINT_ATTRIB(EGL_BIND_TO_TEXTURE_RGB);
			PRINT_ATTRIB(EGL_BIND_TO_TEXTURE_RGBA);
			PRINT_ATTRIB(EGL_BLUE_SIZE);
			PRINT_ATTRIB(EGL_BUFFER_SIZE);
			PRINT_ATTRIB(EGL_COLOR_BUFFER_TYPE);
			PRINT_ATTRIB(EGL_CONFIG_CAVEAT);
			PRINT_ATTRIB(EGL_CONFIG_ID);
			PRINT_ATTRIB(EGL_CONFORMANT);
			PRINT_ATTRIB(EGL_DEPTH_SIZE);
			PRINT_ATTRIB(EGL_GREEN_SIZE);
			PRINT_ATTRIB(EGL_LEVEL);
			PRINT_ATTRIB(EGL_LUMINANCE_SIZE);
			PRINT_ATTRIB(EGL_MAX_PBUFFER_WIDTH);
			PRINT_ATTRIB(EGL_MAX_PBUFFER_HEIGHT);
			PRINT_ATTRIB(EGL_MAX_PBUFFER_PIXELS);
			PRINT_ATTRIB(EGL_MAX_SWAP_INTERVAL);
			PRINT_ATTRIB(EGL_MIN_SWAP_INTERVAL);
			PRINT_ATTRIB(EGL_NATIVE_RENDERABLE);
			PRINT_ATTRIB(EGL_NATIVE_VISUAL_ID);
			PRINT_ATTRIB(EGL_NATIVE_VISUAL_TYPE);
			PRINT_ATTRIB(EGL_RED_SIZE);
			PRINT_ATTRIB(EGL_RENDERABLE_TYPE);
			PRINT_ATTRIB(EGL_SAMPLE_BUFFERS);
			PRINT_ATTRIB(EGL_SAMPLES);
			PRINT_ATTRIB(EGL_STENCIL_SIZE);
			PRINT_ATTRIB(EGL_SURFACE_TYPE);
			PRINT_ATTRIB(EGL_TRANSPARENT_TYPE);
			PRINT_ATTRIB(EGL_TRANSPARENT_RED_VALUE);
			PRINT_ATTRIB(EGL_TRANSPARENT_GREEN_VALUE);
			PRINT_ATTRIB(EGL_TRANSPARENT_BLUE_VALUE);
		}

		#undef PRINT_ATTRIB
#endif

		struct EGLData
		{
			EGLConfig config;
			EGLSurface surface;
			EGLContext context;
		};

		EGLGraphicsProvider::EGLGraphicsProvider() :
			display(EGL_NO_DISPLAY)
		{
		}

		Result EGLGraphicsProvider::startup(WindowProvider* window_provider)
		{
			assert(display == EGL_NO_DISPLAY);

			display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
			assert(display != EGL_NO_DISPLAY);

			int major = 0;
			int minor = 0;
			EGLBoolean initialized = eglInitialize(display, &major, &minor);
			assert(initialized != EGL_FALSE);

			EGLBoolean bind_result = EGL_FALSE;

#if PLATFORM_DEBUG_EGL
			{
				const char* eglstring;
				eglstring = (const char*) eglQueryString(display, EGL_VENDOR);
				PLATFORM_LOG(LogMessageType::Info, "EGL_VENDOR: %s\n", eglstring);

				eglstring = (const char*) eglQueryString(display, EGL_VERSION);
				PLATFORM_LOG(LogMessageType::Info, "EGL_VERSION: %s\n", eglstring);

				eglstring = (const char*) eglQueryString(display, EGL_EXTENSIONS);
				PLATFORM_LOG(LogMessageType::Info, "EGL_EXTENSIONS: %s\n", eglstring);

				if (major >= 1 && minor >= 2)
				{
					eglstring = (const char*) eglQueryString(display, EGL_CLIENT_APIS);
					PLATFORM_LOG(LogMessageType::Info, "EGL_CLIENT_APIS: %s\n", eglstring);
				}
			}
#endif

			// eglBindAPI and EGL version required
			// 1.2+: EGL_OPENGL_ES_API and EGL_OPENVG_API
			// 1.4+: EGL_OPENGL_API
#if defined(PLATFORM_GLES2_SUPPORT)
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

			return Result::success();
		}

		void EGLGraphicsProvider::shutdown(WindowProvider* window_provider)
		{
			assert(display != EGL_NO_DISPLAY);

			detach_context(nullptr);

			eglTerminate(display);
			EGL_CHECK_ERROR("eglTerminate");

			display = EGL_NO_DISPLAY;
		}

		void EGLGraphicsProvider::create_context(NativeWindow* window)
		{
			EGLData* window_data = egldata_from(window);
			assert(window_data);

			// prepare egl attributes
			EGLint attribs[] = {
#if defined(PLATFORM_GLES2_SUPPORT)
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

				// color sizes
				EGL_RED_SIZE, 8,
				EGL_GREEN_SIZE, 8,
				EGL_BLUE_SIZE, 8,

				// alpha size
				EGL_ALPHA_SIZE, 8,

				EGL_NONE
			};

			EGLint total_configs = 0;
			EGLBoolean result = EGL_FALSE;

			// choose one config based on the attributes
			result = eglChooseConfig(display, attribs, &window_data->config, 1, &total_configs);
			assert(result != EGL_FALSE);


			// cache the visual id
			EGLint visual_id;
			result = eglGetConfigAttrib(display, window_data->config, EGL_NATIVE_VISUAL_ID, &visual_id);
			assert(result != EGL_FALSE);

#if EGL_PLATFORM_DEBUG
			egl_print_config(display, window_data->config);
#endif

			// update the underlying window's visual
			window->update_visual(visual_id);

			// create the EGL context
			EGLint context_attribs[] = {
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE,
			};

			EGLContext share_context = EGL_NO_CONTEXT;
			window_data->context = eglCreateContext(display, window_data->config, share_context, context_attribs);
			assert(window_data->context != EGL_NO_CONTEXT);
			EGL_CHECK_ERROR("eglCreateContext");
		}

		void EGLGraphicsProvider::destroy_context(NativeWindow* window)
		{
			EGLData* window_data = egldata_from(window);
			if (window_data->context != EGL_NO_CONTEXT)
			{
				detach_context(window);

				destroy_surface(window);

				EGLBoolean result = eglDestroyContext(display, window_data->context);
				assert(result != EGL_FALSE);
				window_data->context = nullptr;
			}
		}

		void EGLGraphicsProvider::attach_context(NativeWindow* window)
		{
			EGLData* window_data = egldata_from(window);
			assert(window_data->surface != EGL_NO_SURFACE);
			assert(window_data->context != EGL_NO_CONTEXT);

			EGLBoolean result = eglMakeCurrent(display, window_data->surface, window_data->surface, window_data->context);
			EGL_CHECK_ERROR("eglMakeCurrent");
			if (result == EGL_FALSE)
			{
				PLATFORM_LOG(LogMessageType::Info, "eglMakeCurrent failed to activate context\n");
			}
		}

		void EGLGraphicsProvider::detach_context(NativeWindow* window)
		{
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			EGL_CHECK_ERROR("eglMakeCurrent (null)");
		}

		void EGLGraphicsProvider::create_surface(NativeWindow* window)
		{
			EGLData* window_data = egldata_from(window);

			EGLint surface_attributes[] = {
				EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
				EGL_NONE
			};

			window_data->surface = eglCreateWindowSurface(display, window_data->config, (EGLNativeWindowType)window->get_native_handle(), surface_attributes);
			EGL_CHECK_ERROR("eglCreateWindowSurface");

			if (window_data->surface == EGL_NO_SURFACE)
			{
				PLATFORM_LOG(LogMessageType::Info, "eglCreateWindowSurface failed!\n");
				return;
			}

			// update the window's dimensions via EGL
			EGLint width;
			EGLint height;
			eglQuerySurface(display, window_data->surface, EGL_WIDTH, &width);
			eglQuerySurface(display, window_data->surface, EGL_HEIGHT, &height);

			window->update_size(width, height);
		}

		void EGLGraphicsProvider::destroy_surface(NativeWindow* window)
		{
			EGLData* window_data = egldata_from(window);
			if (window_data->surface != EGL_NO_SURFACE)
			{
				EGLBoolean result = EGL_FALSE;
				result = eglDestroySurface(display, window_data->surface);
				assert(result != EGL_FALSE);

				window_data->surface = EGL_NO_SURFACE;
			}
		}

		void EGLGraphicsProvider::swap_buffers(NativeWindow* window)
		{
			EGLData* window_data = egldata_from(window);
			eglSwapBuffers(display, window_data->surface);
			EGL_CHECK_ERROR("eglSwapBuffers");
		}

		void* EGLGraphicsProvider::get_symbol(const char* symbol_name)
		{
			return nullptr;
		}

		size_t EGLGraphicsProvider::get_graphics_data_size() const
		{
			return sizeof(EGLData);
		}

		void EGLGraphicsProvider::pre_window_creation(const Parameters& window_parameters, void* graphics_data)
		{

		}

		void* EGLGraphicsProvider::get_native_visual(void* graphics_data)
		{
			return nullptr;
		}

		EGLData* EGLGraphicsProvider::egldata_from(NativeWindow* window)
		{
			assert(window != nullptr);
			return reinterpret_cast<EGLData*>(window->graphics_data);
		}
	} // namespace window

} // namespace platform

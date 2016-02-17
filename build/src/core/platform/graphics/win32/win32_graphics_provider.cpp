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
#include "win32_graphics_provider.h"
#include <window/win32/win32_window_provider.h>
#include <window/win32/win32_window.h>

#include <gl/GL.h>

#include <core/logging.h>

#include <assert.h>

#define PLATFORM_DEBUG_WIN32 0

namespace platform
{
	namespace window
	{
#if 0
		#define GLX_CONTEXT_PROFILE_MASK_ARB 					0x9126
		#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB				0x00000001
		#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB		0x00000002

		typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
		GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;

		int _global_error_code = 0;

		int error_handler(Display* display, XErrorEvent* event)
		{
			LOGE("XErrorEvent->error_code: %i\n", event->error_code);
			_global_error_code = event->error_code;
			return 0;
		}

		const size_t WINDOW_MAX_ATTRIBUTES = 24;

#endif
		// constants for wglChoosePixelFormatARB
		const uint32_t WGL_DRAW_TO_WINDOW_ARB						= 0x2001;
		const uint32_t WGL_SUPPORT_OPENGL_ARB						= 0x2010;
		const uint32_t WGL_DOUBLE_BUFFER_ARB						= 0x2011;
		const uint32_t WGL_PIXEL_TYPE_ARB							= 0x2013;
		const uint32_t WGL_COLOR_BITS_ARB							= 0x2014;
		const uint32_t WGL_DEPTH_BITS_ARB							= 0x2022;
		const uint32_t WGL_STENCIL_BITS_ARB							= 0x2023;
		const uint32_t WGL_SAMPLE_BUFFERS_ARB						= 0x2041;
		const uint32_t WGL_SAMPLES_ARB								= 0x2042;

		const uint32_t WGL_TYPE_RGBA_ARB							= 0x202B;


		// constants for wglCreateContextAttribsARB
		const uint32_t WGL_CONTEXT_MAJOR_VERSION_ARB				= 0x2091;
		const uint32_t WGL_CONTEXT_MINOR_VERSION_ARB				= 0x2092;
		const uint32_t WGL_CONTEXT_LAYER_PLANE_ARB					= 0x2093;
		const uint32_t WGL_CONTEXT_FLAGS_ARB						= 0x2094;
		const uint32_t WGL_PROFILE_MASK_ARB							= 0x9126;

		const uint32_t WGL_CONTEXT_DEBUG_BIT_ARB					= 0x0001;
		const uint32_t WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB		= 0x0002;
		const uint32_t WGL_CONTEXT_CORE_PROFILE_BIT_ARB				= 0x00000001;
		const uint32_t WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	= 0x00000002;


		struct Win32GraphicsData
		{
			HDC device_context;
			HGLRC render_context;
		};

		static Win32GraphicsData* data_from(NativeWindow* window)
		{
			return reinterpret_cast<Win32GraphicsData*>(window->graphics_data);
		}

		// typedefs from the ARB extension registry.
		// See:
		// https://www.opengl.org/registry/specs/ARB/wgl_pixel_format.txt
		// https://www.opengl.org/registry/specs/ARB/wgl_create_context.txt

		typedef BOOL (__stdcall* wgl_choose_pixel_format)(
			HDC device_context,
			const int* attribute_int_list,
			const FLOAT* attribute_float_list,
			UINT max_formats,
			int* format_indices,
			UINT* total_matching_formats
		);

		typedef HGLRC (__stdcall* wgl_create_context_attribs)(
			HDC device_context,
			HGLRC share_context,
			const int* attribute_list
		);

		wgl_choose_pixel_format choose_pixel_format = nullptr;
		wgl_create_context_attribs create_context_attribs = nullptr;


		Win32GraphicsProvider::Win32GraphicsProvider() :
			share_context(nullptr)
		{
		}

		Result Win32GraphicsProvider::startup(WindowProvider* wp)
		{
			window_provider = static_cast<Win32WindowProvider*>(wp);
			assert(window_provider != nullptr);

			// We need to create a temporary window to grab the
			// CreateContextAttribsARB symbol.
			Parameters params;
			params.frame.width = 100;
			params.frame.height = 100;
			params.window_title = "temp";
			NativeWindow* temporary_window = ::platform::window::create(params);
			assert(temporary_window);
			activate_context(temporary_window);

			// At this point, we should have a valid GL context.
			// Attempt to load symbols...
			choose_pixel_format = (wgl_choose_pixel_format)get_symbol("wglChoosePixelFormatARB");
			assert(choose_pixel_format);

			create_context_attribs = (wgl_create_context_attribs)get_symbol("wglCreateContextAttribsARB");
			assert(create_context_attribs);

			// Now that we've loaded the symbols, we can destroy that window
			// and create a new one with the symbols we fetched. Windows doesn't
			// allow you to change the pixel format on a window once created.

			platform::window::destroy(temporary_window, DestroyWindowBehavior::WitholdDestroyMessage);

			return Result::success();
		}

		void Win32GraphicsProvider::shutdown(WindowProvider* /*window_provider*/)
		{
		}

		void Win32GraphicsProvider::create_context(NativeWindow* window)
		{
			// Choose the correct pixel format for window.
			Win32GraphicsData* data = data_from(window);

			win32::Window* native_window = static_cast<win32::Window*>(window);
			data->device_context = GetDC(static_cast<HWND>(native_window->get_native_handle()));

			if (choose_pixel_format)
			{
				int attributes[] = {
					WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
					WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
					WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
					WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB, 32,
					WGL_DEPTH_BITS_ARB, 24,
					WGL_STENCIL_BITS_ARB, 8,
					0
				};

				const UINT max_formats = 1;
				int format_indices[1];
				UINT total_matching_formats;

				BOOL result = choose_pixel_format(data->device_context,
					attributes,
					nullptr,
					max_formats,
					format_indices,
					&total_matching_formats
				);

				if (result == FALSE)
				{
					LOGE("Unable to choose a valid pixel format! Error [ARB]: %i\n", GetLastError());
					return;
				}

				result = SetPixelFormat(data->device_context, format_indices[0], nullptr);
				if (result == FALSE)
				{
					LOGE("SetPixelFormat failed! Error [ARB]: %i\n", GetLastError());
					return;
				}

				// choose attributes
				int context_attributes[] = {
					WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
					WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
					WGL_CONTEXT_MINOR_VERSION_ARB, 2,
					0
				};

				data->render_context = create_context_attribs(data->device_context,
					nullptr,
					context_attributes
				);
			}
			else
			{
				// fall back to the default way of choosing a pixel format
				PIXELFORMATDESCRIPTOR pfd;
				memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
				pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
				pfd.nVersion = 1;
				pfd.dwFlags = (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER);
				pfd.cColorBits = 32;
				pfd.cDepthBits = 24;
				pfd.cStencilBits = 8;
				pfd.dwLayerMask = PFD_MAIN_PLANE;
				pfd.iPixelType = PFD_TYPE_RGBA;

				int32_t pixel_format = ChoosePixelFormat(data->device_context, &pfd);
				if (pixel_format == 0)
				{
					LOGE("Unable to choose a valid pixel format! Error: %i\n", GetLastError());
					return;
				}

				BOOL set_pixel_success = SetPixelFormat(data->device_context, pixel_format, &pfd);
				assert(set_pixel_success);

				// create the OpenGL context
				data->render_context = wglCreateContext(data->device_context);
			}

			assert(data->render_context);
#if 0
			X11GraphicsData* data = data_from(window->graphics_data);
			if (glXCreateContextAttribsARB)
			{
				int attributes[WINDOW_MAX_ATTRIBUTES];
				attributes_from_backbuffer(attributes, WINDOW_MAX_ATTRIBUTES, window->backbuffer);

				// create the GLXFBConfig; added in GLX version 1.3
				int total_elements = 0;
				GLXFBConfig* config = glXChooseFBConfig(
					window_provider->get_display(),
					DefaultScreen(window_provider->get_display()),
					attributes,
					&total_elements
				);


				int context_attributes[] = {
					GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
					GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
					GLX_CONTEXT_MINOR_VERSION_ARB, 2,
					None
				};

				bool direct_rendering = true;
				data->context = glXCreateContextAttribsARB(
					window_provider->get_display(),
					*config,
					share_context,
					direct_rendering,
					context_attributes
				);

				// fall back to compatibility mode
				if (_global_error_code != 0)
				{
					_global_error_code = 0;
					context_attributes[1] = GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
					context_attributes[2] = None;
					data->context = glXCreateContextAttribsARB(
						window_provider->get_display(),
						*config,
						share_context,
						direct_rendering,
						context_attributes
					);

					// We can't even get a 2.1 context!
					assert(data->context != nullptr);
				}


				if (!share_context)
				{
					share_context = data->context;
				}

				assert(glXIsDirect(window_provider->get_display(), data->context));
			}
			else // fallback and use glXCreateContext
			{
				data->context = glXCreateContext(window_provider->get_display(), data->vi, nullptr, GL_TRUE);
			}

			assert(data->context);
#endif
		}

		void Win32GraphicsProvider::destroy_context(NativeWindow* window)
		{
			Win32GraphicsData* data = data_from(window);
			wglDeleteContext(data->render_context);

			ReleaseDC(static_cast<HWND>(window->get_native_handle()), data->device_context);
		}

		void Win32GraphicsProvider::attach_context(NativeWindow* window)
		{
			Win32GraphicsData* data = data_from(window);
			wglMakeCurrent(data->device_context, data->render_context);
#if 0
			BOOL result = wglMakeCurrent(data->device_context, data->render_context);
			if (!result)
			{
				DWORD last_error = GetLastError();
				assert(last_error = 0);
			}
			assert(result);
#endif
		}

		void Win32GraphicsProvider::detach_context(NativeWindow* window)
		{
			Win32GraphicsData* data = data_from(window);
			wglMakeCurrent(data->device_context, nullptr);
		}

		void Win32GraphicsProvider::create_surface(NativeWindow*)
		{
		}

		void Win32GraphicsProvider::destroy_surface(NativeWindow*)
		{
		}

		void Win32GraphicsProvider::swap_buffers(NativeWindow* window)
		{
			Win32GraphicsData* data = data_from(window);
			SwapBuffers(data->device_context);
		}

		void* Win32GraphicsProvider::get_symbol(const char* symbol_name)
		{
			return wglGetProcAddress(symbol_name);
		}

		size_t Win32GraphicsProvider::get_graphics_data_size() const
		{
			return sizeof(Win32GraphicsData);
		}

		void Win32GraphicsProvider::pre_window_creation(const Parameters&, void*)
		{
		}

		void* Win32GraphicsProvider::get_native_visual(void*)
		{
			return nullptr;
		}
	} // namespace window

} // namespace platform

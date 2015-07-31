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

#include <assert.h>

#define PLATFORM_DEBUG_X11 0

namespace platform
{
	namespace window
	{
		#define GLX_CONTEXT_PROFILE_MASK_ARB 					0x9126
		#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB				0x00000001
		#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB		0x00000002

		typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
		GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;


		const size_t WINDOW_MAX_ATTRIBUTES = 24;

		struct X11GraphicsData
		{
			XVisualInfo* vi;
			GLXContext context;

			X11GraphicsData() : 
				vi(nullptr)
			{		
			}
		};

		static X11GraphicsData* data_from(void* data)
		{
			return reinterpret_cast<X11GraphicsData*>(data);
		}

		X11GraphicsProvider::X11GraphicsProvider() :
			share_context(nullptr)
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


			// We must create a temporary window to grab the GL context
			Parameters params;
			params.frame.width = 100;
			params.frame.height = 100;
			params.window_title = "temp";

			NativeWindow* temporary_window = ::platform::window::create(params);

			// We should have a valid GL context now -- let's try to fetch the 

			PLATFORM_LOG(LogMessageType::Info, "fetching symbol\n");

			const GLubyte context_attribs_arb[] = "glXCreateContextAttribsARB";
			glXCreateContextAttribsARB = reinterpret_cast<GLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddress(context_attribs_arb));

			// failed, try without ARB?
			if (!glXCreateContextAttribsARB)
			{
				PLATFORM_LOG(LogMessageType::Warning, "Unable to find symbol 'glXCreateContextAttribsARB'");
				const GLubyte context_attribs[] = "glXCreateContextAttribs";
				glXCreateContextAttribsARB = reinterpret_cast<GLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddress(context_attribs));
			}

			::platform::window::destroy(temporary_window);

			return Result::success();
		}

		void X11GraphicsProvider::shutdown(WindowProvider* window_provider)
		{
		}

		void X11GraphicsProvider::create_context(NativeWindow* window)
		{
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
		}

		void X11GraphicsProvider::destroy_context(NativeWindow* window)
		{
			X11GraphicsData* data = data_from(window->graphics_data);
			glXDestroyContext(window_provider->get_display(), data->context);
		}

		void X11GraphicsProvider::attach_context(NativeWindow* window)
		{
			X11GraphicsData* data = data_from(window->graphics_data);
			void* native_handle = window->get_native_handle();
			Window* window_handle = static_cast<Window*>(native_handle);

			glXMakeCurrent(window_provider->get_display(), *window_handle, data->context);
		}

		void X11GraphicsProvider::detach_context(NativeWindow* window)
		{
			glXMakeCurrent(window_provider->get_display(), None, nullptr);
		}

		void X11GraphicsProvider::create_surface(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::destroy_surface(NativeWindow* window)
		{
		}

		void X11GraphicsProvider::swap_buffers(NativeWindow* window)
		{
			void* native_handle = window->get_native_handle();
			Window* window_handle = static_cast<Window*>(native_handle);			
			glXSwapBuffers(window_provider->get_display(), *window_handle);
		}

		void* X11GraphicsProvider::get_symbol(const char* symbol_name)
		{
			return nullptr;
		}

		size_t X11GraphicsProvider::get_graphics_data_size() const
		{
			return sizeof(X11GraphicsData);
		}

		void X11GraphicsProvider::pre_window_creation(const Parameters& window_parameters, void* graphics_data)
		{
			X11GraphicsData* data = static_cast<X11GraphicsData*>(graphics_data);

			int attributes[WINDOW_MAX_ATTRIBUTES];
			attributes_from_backbuffer(attributes, WINDOW_MAX_ATTRIBUTES, window_parameters.backbuffer);

			// create the GLXFBConfig; added in GLX version 1.3
			int total_elements = 0;
			GLXFBConfig* config = glXChooseFBConfig(
				window_provider->get_display(),
				DefaultScreen(window_provider->get_display()),
				attributes,
				&total_elements
			);

			assert(config != nullptr);

			data->vi = glXGetVisualFromFBConfig(
				window_provider->get_display(), 
				*config
			);
		}

		void* X11GraphicsProvider::get_native_visual(void* graphics_data)
		{
			X11GraphicsData* data = data_from(graphics_data);
			return data->vi->visual;
		}


		void X11GraphicsProvider::attributes_from_backbuffer(int* attributes, size_t total_attributes, const BackbufferConfig& backbuffer)
		{
			size_t index = 0;

			// only consider double-buffered frame configurations
			attributes[index++] = GLX_DOUBLEBUFFER;
			attributes[index++] = True;

			// only frame buffer configurations that have associated X visuals
			attributes[index++] = GLX_X_RENDERABLE;
			attributes[index++] = True;

			// setup the back buffer for the window
			attributes[index++] = GLX_RED_SIZE;
			attributes[index++] = backbuffer.red_size;
			attributes[index++] = GLX_GREEN_SIZE;
			attributes[index++] = backbuffer.green_size;
			attributes[index++] = GLX_BLUE_SIZE;
			attributes[index++] = backbuffer.blue_size;
			attributes[index++] = GLX_ALPHA_SIZE;
			attributes[index++] = backbuffer.alpha_size;
			attributes[index++] = GLX_DEPTH_SIZE;
			attributes[index++] = backbuffer.depth_size;
			attributes[index++] = GLX_STENCIL_SIZE;
			attributes[index++] = backbuffer.stencil_size;

			attributes[index++] = None;

			// overflow
			assert(index < total_attributes);
		}

	} // namespace window

} // namespace platform

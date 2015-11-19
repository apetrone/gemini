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
#include "window_provider.h"
#include "graphics_provider.h"

#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#if defined(PLATFORM_RASPBERRYPI)
	#include <bcm_host.h> // for bcm_host_init

	#include "../../window/dispmanx/dispmanx_window_provider.h"
#endif



#if defined(PLATFORM_EGL_SUPPORT)
	#include "../../graphics/egl/egl_graphics_provider.h"
#endif

#if defined(PLATFORM_X11_SUPPORT)
	#include "../../window/x11/x11_window_provider.h"
	#include "../../graphics/x11/x11_graphics_provider.h"
#endif

#include <stdio.h> // for fprintf

using namespace platform::window;

namespace platform
{
	namespace linux
	{
		static WindowProvider* _window_provider = nullptr;
		static GraphicsProvider* _graphics_provider = nullptr;


		// choose the best window provider
		WindowProvider* create_window_provider()
		{
#if defined(PLATFORM_RASPBERRYPI)
			typedef DispManXWindowProvider window_provider_type;
#elif defined(PLATFORM_X11_SUPPORT)
			typedef X11WindowProvider window_provider_type;
#else
			#error No window provider for this platform!
#endif

			return MEMORY_NEW(window_provider_type, get_platform_allocator());
		}

		GraphicsProvider* create_graphics_provider()
		{
#if defined(PLATFORM_RASPBERRYPI)
			typedef EGLGraphicsProvider graphics_provider_type;
#elif defined(PLATFORM_X11_SUPPORT)
			typedef X11GraphicsProvider graphics_provider_type;
#else
			#error No graphics provider for this platform!
#endif

			return MEMORY_NEW(graphics_provider_type, get_platform_allocator());
		}
	} // namespace linux


	using namespace platform::linux;

	Result backend_startup()
	{
		return Result::success();
	}

	int backend_run_application(int argc, const char** argv)
	{
		return 0;
	}

	void backend_shutdown()
	{
	}

	void backend_log(platform::LogMessageType type, const char* message)
	{
		FILE* log_message_to_pipe[] = {
			stdout,
			stdout,
			stderr
		};

		fprintf(log_message_to_pipe[static_cast<int>(type)], message);
	}


	void dispatch_events()
	{

	}


	namespace window
	{
		Result startup(RenderingBackend backend)
		{
			// On linux, the platform layer needs to be versatile.
			// It is split into three main components I call 'providers':
			// 1. Window: platform window management layer
			// 2. Graphics: rendering context provider
			// 3. Input: various input systems

			// Ultimately, we have to choose the best window provider
			// either via build settings or at runtime.

#if defined(PLATFORM_RASPBERRYPI)
			fprintf(stdout, "[Raspberry Pi]!\n");

			// this must be called before we can issue any hardware commands
			bcm_host_init();
#endif

			// window provider
			assert(_window_provider == nullptr);
			_window_provider = create_window_provider();
			if (!_window_provider)
			{
				return Result::failure("create_window_provider failed!");
			}

			Result window_startup = _window_provider->startup();
			if (window_startup.failed())
			{
				fprintf(stderr, "window_provider startup failed: %s\n", window_startup.message);
				return window_startup;
			}

			//
			// graphics provider
			assert(_graphics_provider == nullptr);
			_graphics_provider = create_graphics_provider();
			if (!_graphics_provider)
			{
				return Result::failure("create_graphics_provider failed!");
			}

			Result graphics_startup = _graphics_provider->startup(_window_provider);
			if (graphics_startup.failed())
			{
				fprintf(stderr, "graphics_provider startup failed: %s\n", graphics_startup.message);
				return graphics_startup;
			}

			//
			// input provider



#if defined(PLATFORM_RASPBERRYPI)
			// force the backend
			backend = RenderingBackend_OpenGLES2;

			if (backend != RenderingBackend_OpenGLES2)
			{
				return Result::failure("The only supported rendering backend is OpenGL ES 2");
			}
#else
			// force OpenGL for now
			backend = RenderingBackend_OpenGL;
			if (backend != RenderingBackend_OpenGL)
			{
				return Result::failure("Only the OpenGL rendering backend is supported");
			}
#endif
			return Result::success();
		}

		void shutdown()
		{
			assert(_graphics_provider != nullptr);
			_graphics_provider->shutdown(_window_provider);
			MEMORY_DELETE(_graphics_provider, get_platform_allocator());

			assert(_window_provider != nullptr);
			_window_provider->shutdown();
			MEMORY_DELETE(_window_provider, get_platform_allocator());
			_window_provider = nullptr;

#if defined(PLATFORM_RASPBERRYPI)
			bcm_host_deinit();
#endif
		}

		void dispatch_events()
		{

		}

		NativeWindow* create(const Parameters& window_parameters)
		{
			// allocate data for the graphics provider
			size_t graphics_data_size = _graphics_provider->get_graphics_data_size();
			void* graphics_data = nullptr;

			if (graphics_data_size)
			{
				// alloc graphics data for this window
				graphics_data = MEMORY_ALLOC(graphics_data_size, get_platform_allocator());
			}

			// have the graphics provider figure out what it may need prior
			// to window creation.
			_graphics_provider->pre_window_creation(window_parameters, graphics_data);

			// create the native window and assign the graphics data
			NativeWindow* window = _window_provider->create(window_parameters, _graphics_provider->get_native_visual(graphics_data));
			window->graphics_data = graphics_data;


			window->backbuffer = window_parameters.backbuffer;

			// pass the window to the graphics API for context creation
			_graphics_provider->create_context(window);

			// another pass to create the 'surface'
			_graphics_provider->create_surface(window);

			// activate the context for newly created windows
			_graphics_provider->attach_context(window);

			return window;
		}

		void destroy(NativeWindow* window)
		{
			_graphics_provider->detach_context(window);
			_graphics_provider->destroy_surface(window);
			_graphics_provider->destroy_context(window);

			MEMORY_DEALLOC(window->graphics_data, get_platform_allocator());
			_window_provider->destroy(window);
		}

		void activate_context(NativeWindow* window)
		{
			_graphics_provider->attach_context(window);
		}

		void deactivate_context(NativeWindow* window)
		{
			_graphics_provider->detach_context(window);
		}

		void swap_buffers(NativeWindow* window)
		{
			_graphics_provider->swap_buffers(window);
		}

		Frame get_frame(NativeWindow* window)
		{
			return _window_provider->get_frame(window);
		}

		Frame get_render_frame(NativeWindow* window)
		{
			return _window_provider->get_render_frame(window);
		}

		size_t screen_count()
		{
			return _window_provider->get_screen_count();
		}

		Frame screen_frame(size_t screen_index)
		{
			return _window_provider->get_screen_frame(screen_index);
		}

		void focus(NativeWindow* window)
		{
		}

		void show_cursor(bool enable)
		{
		}

		void set_cursor(float x, float y)
		{

		}

		void get_cursor(float& x, float& y)
		{

		}

		void set_relative_mouse_mode(bool enable)
		{

		}

		void set_mouse_tracking(bool enable)
		{

		}
	} // namespace window


	size_t system_pagesize_bytes()
	{
		return sysconf(_SC_PAGESIZE);
	}

	size_t system_processor_count()
	{
		return sysconf(_SC_NPROCESSORS_ONLN);
	}

	double system_uptime_seconds()
	{
		// kernel 2.6+ compatible
		struct sysinfo system_info;
		assert(0 == sysinfo(&system_info));
		return system_info.uptime;
	}

	core::StackString<64> system_version_string()
	{
		struct utsname name;
		assert(0 == uname(&name));

		core::StackString<64> version = name.sysname;
		version.append(" ");
		version.append(name.release);

		return version;
	}

} // namespace platform


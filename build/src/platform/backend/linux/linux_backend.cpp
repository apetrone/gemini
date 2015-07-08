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
	#include <bcm_host.h> // for bcm_host_init

	#include "../../window/dispmanx/dispmanx_window_provider.h"
#endif


#if defined(PLATFORM_EGL_SUPPORT)
	#include "../../graphics/egl/egl_graphics_provider.h"
#endif

namespace platform
{
	namespace linux
	{
		WindowProvider::~WindowProvider()
		{
		}

		GraphicsProvider::~GraphicsProvider()
		{
		}

		static WindowProvider* _window_provider = nullptr;
		static GraphicsProvider* _graphics_provider = nullptr;


		// choose the best window provider
		WindowProvider* create_window_provider()
		{
#if defined(PLATFORM_RASPBERRYPI)
			typedef DispManXWindowProvider window_provider_type;
#else
			#error No window provider for this platform!
#endif

			return MEMORY_NEW(window_provider_type, get_platform_allocator());
		}

		GraphicsProvider* create_graphics_provider()
		{
#if defined(PLATFORM_RASPBERRYPI)
			typedef EGLGraphicsProvider graphics_provider_type;
#else
			#error No graphics provider for this platform!
#endif

			return MEMORY_NEW(graphics_provider_type, get_platform_allocator());
		}
	} // namespace linux


	using namespace platform::linux;

	Result backend_startup()
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

		//
		// graphics provider
		assert(_graphics_provider == nullptr);
		_graphics_provider = create_graphics_provider();
		if (!_graphics_provider)
		{
			return Result(Result::Failure, "create_graphics_provider failed!");
		}

		Result graphics_startup = _graphics_provider->startup();
		if (graphics_startup.failed())
		{
			fprintf(stderr, "graphics_provider startup failed: %s\n", graphics_startup.message);
			return graphics_startup;
		}

		// window provider
		assert(_window_provider == nullptr);
		_window_provider = create_window_provider();
		if (!_window_provider)
		{
			return Result(Result::Failure, "create_window_provider failed!");
		}

		Result window_startup = _window_provider->startup();
		if (window_startup.failed())
		{
			fprintf(stderr, "window_provider startup failed: %s\n", window_startup.message);
			return window_startup;
		}

		//
		// input provider
		

		return Result(Result::Success);
	}
	
	int backend_run_application(int argc, const char** argv)
	{
		return 0;
	}

	void backend_shutdown()
	{
		assert(_window_provider != nullptr);
		_window_provider->shutdown();
		MEMORY_DELETE(_window_provider, get_platform_allocator());
		_window_provider = nullptr;

		assert(_graphics_provider != nullptr);
		_graphics_provider->shutdown();
		MEMORY_DELETE(_graphics_provider, get_platform_allocator());

#if defined(PLATFORM_RASPBERRYPI)
		bcm_host_deinit();
#endif		
	}


	void dispatch_events()
	{

	}



	NativeWindow* window_create(const WindowParameters& window_parameters)
	{
		NativeWindow* window = _window_provider->create(window_parameters);

		size_t graphics_data_size = _graphics_provider->get_graphics_data_size();

		if (graphics_data_size)
		{
			// alloc graphics data for this window
			window->graphics_data = MEMORY_ALLOC(graphics_data_size, get_platform_allocator());
		}


		// pass the window to the graphics API
		_graphics_provider->create_context(window);

		// activate the context
		_graphics_provider->begin_rendering(window);

		return window;
	}

	void window_destroy(NativeWindow* window)
	{
		_graphics_provider->destroy_context(window);

		MEMORY_DEALLOC(window->graphics_data, get_platform_allocator());
		_window_provider->destroy(window);
	}
	
	void window_begin_rendering(NativeWindow* window)
	{
		_graphics_provider->begin_rendering(window);
	}
	
	void window_end_rendering(NativeWindow* window)
	{
		_graphics_provider->end_rendering(window);
	}


	void window_size(NativeWindow* window, int& width, int& height)
	{
	}
	
	void window_render_size(NativeWindow* window, int& width, int& height)
	{
	}
	
	size_t window_screen_count()
	{
		return _window_provider->get_screen_count();
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
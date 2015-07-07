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

namespace platform
{
	namespace linux
	{
		window_provider::~window_provider()
		{
		}


		static window_provider* _window_provider = nullptr;


		// choose the best window provider
		window_provider* create_window_provider()
		{
#if defined(PLATFORM_RASPBERRYPI)
			typedef dispmanx_window_provider window_provider_type;
#else
			#error No window provider for this platform!
#endif

			return MEMORY_NEW(window_provider_type, platform::get_platform_allocator());
		}

		void destroy_window_provider(window_provider* provider)
		{
			MEMORY_DELETE(provider, platform::get_platform_allocator());
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


		assert(_window_provider == nullptr);
		_window_provider = create_window_provider();
		if (!_window_provider)
		{
			return Result(Result::Failure, "create_window_provider failed!");
		}

		_window_provider->startup();

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
		destroy_window_provider(_window_provider);
		_window_provider = nullptr;
	}


	void dispatch_events()
	{

	}



	NativeWindow* window_create(const WindowParameters& window_parameters)
	{
		return _window_provider->create(window_parameters);
	}

	void window_destroy(NativeWindow* window)
	{
		_window_provider->destroy(window);
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
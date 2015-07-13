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
#include "android_backend.h"
#include "graphics_provider.h"
#include "../../window/android/android_window_provider.h"

#include <android_native_app_glue.h>

// #include <android/sensor.h>
// #include <android/asset_manager.h>
// 



#include <android/log.h>

#include <android/native_window_jni.h> // for ANativeWindow_fromSurface
#include <jni.h>

#if defined(PLATFORM_EGL_SUPPORT)
	#include "../../graphics/egl/egl_graphics_provider.h"
#endif

#define NATIVE_LOG( fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "gemini", fmt, ##__VA_ARGS__)

#include <platform/platform.h>
#include <platform/kernel.h>
#include <core/mem.h>

namespace detail
{
	struct AndroidState
	{
		android_app* app;
		JavaVM* vm;
		// ASensorManager* sensor_manager;
		// ASensorEventQueue* sensor_event_queue;
		
	}; // AndroidState

	static AndroidState _state;
} // namespace detail


using namespace platform::window;

namespace platform
{
	namespace android
	{
		static WindowProvider* _window_provider = nullptr;
		static GraphicsProvider* _graphics_provider = nullptr;


		// choose the best window provider
		WindowProvider* create_window_provider()
		{
			typedef AndroidWindowProvider window_provider_type;
			return MEMORY_NEW(window_provider_type, get_platform_allocator());
		}

		GraphicsProvider* create_graphics_provider()
		{
			typedef EGLGraphicsProvider graphics_provider_type;
			return MEMORY_NEW(graphics_provider_type, get_platform_allocator());
		}
	} // namespace android


	using namespace platform::android;

	Result backend_startup()
	{
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
	
	void android_handle_command(struct android_app* app, int32_t command)
	{
		// Fill this in with the function to process main app commands (APP_CMD_*)
		
		switch(command)
		{
			case APP_CMD_INPUT_CHANGED: break;

			// The window is being shown
			case APP_CMD_INIT_WINDOW:
				NATIVE_LOG("APP_CMD_INIT_WINDOW\n");
				break;

			// The window is being hidden or closed
			case APP_CMD_TERM_WINDOW:
				NATIVE_LOG("APP_CMD_TERM_WINDOW\n");
				break;

			case APP_CMD_WINDOW_RESIZED: break;
			case APP_CMD_WINDOW_REDRAW_NEEDED: break;
			case APP_CMD_CONTENT_RECT_CHANGED: break;

			// The app gains focus
			case APP_CMD_GAINED_FOCUS:
				NATIVE_LOG("APP_CMD_GAINED_FOCUS\n");
				break;

			// The app lost focus
			case APP_CMD_LOST_FOCUS:
				NATIVE_LOG("APP_CMD_LOST_FOCUS\n");
				break;

			case APP_CMD_CONFIG_CHANGED: break;
			case APP_CMD_LOW_MEMORY: break;
			case APP_CMD_START: break;
			case APP_CMD_RESUME: break;

			// The system has asked the application to save its state
			case APP_CMD_SAVE_STATE:
				NATIVE_LOG("APPCMD_SAVE_STATE\n");
				break;

			case APP_CMD_PAUSE: break;
			case APP_CMD_STOP: break;
			case APP_CMD_DESTROY: break;
		}
	}

	static int32_t android_handle_input(struct android_app* app, AInputEvent* event)
	{
		// Fill this in with the function to process input events.  At this point
		// the event has already been pre-dispatched, and it will be finished upon
		// return.  Return 1 if you have handled the event, 0 for any default
		// dispatching.
		return 0;
	}


	// android_main runs in its own thread with its own event loop
	// for receiving input events.
	int backend_run_application(android_app* app)
	{
		// don't strip the native glue code
		app_dummy();

		detail::_state.app = app;
		detail::_state.vm = app->activity->vm;

		// setup the callbacks
		app->onAppCmd = android_handle_command;
		app->onInputEvent = android_handle_input;

		platform::startup();

		// attempt kernel startup
		kernel::Error error = kernel::startup();
		if (error != kernel::NoError)
		{
			NATIVE_LOG("Kernel startup failed with kernel code: %i\n", error);
			kernel::shutdown();
			return -1;
		}

		while(kernel::instance() && kernel::instance()->is_active())
		{
			// read all pending events
			int ident;
			int events;
			struct android_poll_source* source;

			int method = 0;
			// -1: block and wait for new events
			// 0: loop all events and then advance
			
			while((ident = ALooper_pollAll(method, nullptr, &events, (void**)&source)) >= 0)
			{
				if (source)
				{
					source->process(app, source);
				}

				if (app->destroyRequested != 0)
				{
					NATIVE_LOG("android state requested destroy!\n");
					kernel::instance()->set_active(false);
					break;
				}
			}	

			// next frame
			kernel::instance()->tick();
		}

		NATIVE_LOG("exiting the android application\n");

		// cleanup kernel memory
		kernel::shutdown();

		// shutdown the platform
		platform::shutdown();

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
	}


	void dispatch_events()
	{
	}


	namespace window
	{
		Result startup(RenderingBackend backend)
		{
			// force the backend
			backend = RenderingBackend_OpenGLES2;

			if (backend != RenderingBackend_OpenGLES2)
			{
				return Result(Result::Failure, "The only supported rendering backend is OpenGL ES 2");
			}

			return Result(Result::Success);
		}

		void shutdown()
		{

		}

		void dispatch_events()
		{

		}

		NativeWindow* create(const Parameters& window_parameters)
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

			// activate the context for newly created windows
			_graphics_provider->activate_context(window);

			return window;
		}

		void destroy(NativeWindow* window)
		{
			_graphics_provider->destroy_context(window);

			MEMORY_DEALLOC(window->graphics_data, get_platform_allocator());
			_window_provider->destroy(window);
		}
		
		void begin_rendering(NativeWindow* window)
		{
			_graphics_provider->activate_context(window);
		}
		
		void end_rendering(NativeWindow* window)
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

		ANativeWindow* get_android_window()
		{
			return detail::_state.app->window;
		}
	} // namespace window
} // namespace platform

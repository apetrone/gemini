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
#include "../../graphics/egl/egl_graphics_provider.h"

#include <platform/platform.h>
#include <platform/kernel.h>
#include <core/mem.h>
#include <core/logging.h>
#include <core/core.h>

#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/window.h>
#include <android/native_activity.h>
#include <android/api-level.h>
#include <android/input.h>

#include <unistd.h>

#ifndef __ANDROID_API__
	#error __ANDROID_API__ was not found!
#endif

// If you hit this assert, you've attempted to compile with an
// unsupported NDK version.
static_assert(__ANDROID_API__ >= 21, "Android API level < 21");

#include <jni.h>

namespace detail
{
	struct AndroidState
	{
		android_app* app;
		JavaVM* vm;
		bool is_running; // application is running
		bool kernel_started;	// kernel::startup has executed

		bool is_resumed; // app is resumed
		bool has_focus;	// window has focus
		bool surface_ready; // surface is ready

		bool egl_valid;	// egl objects are valid

		AndroidState() :
			is_running(true),
			kernel_started(false),
			is_resumed(false),
			has_focus(false),
			surface_ready(false),
			egl_valid(false)
		{
		}
	}; // AndroidState
} // namespace detail


using namespace platform::window;

namespace platform
{
	namespace android
	{
		static detail::AndroidState* _state;
		static AndroidWindowProvider* _window_provider = nullptr;
		static EGLGraphicsProvider* _graphics_provider = nullptr;


		// choose the best window provider
		AndroidWindowProvider* create_window_provider()
		{
			typedef AndroidWindowProvider window_provider_type;
			return MEMORY_NEW(window_provider_type, get_platform_allocator());
		}

		EGLGraphicsProvider* create_graphics_provider()
		{
			typedef EGLGraphicsProvider graphics_provider_type;
			return MEMORY_NEW(graphics_provider_type, get_platform_allocator());
		}
	} // namespace android


	using namespace platform::android;

	Result backend_startup()
	{
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


		// Initialize the AndroidWindow here.
		AndroidWindow* window = _window_provider->get_android_window();
		assert(window != nullptr);

		size_t graphics_data_size = _graphics_provider->get_graphics_data_size();
		if (graphics_data_size)
		{
			// alloc graphics data for this window
			window->graphics_data = MEMORY_ALLOC(graphics_data_size, get_platform_allocator());
			memset(window->graphics_data, 0, sizeof(graphics_data_size));
		}

		LOGV("AndroidWindow: %p\n", window);

		// create a context for the window; through EGL this doesn't require
		// a valid window surface -- as we don't have one yet.
		_graphics_provider->create_context(window);

		return Result::success();
	}

	void android_handle_command(struct android_app* app, int32_t command)
	{
		// Fill this in with the function to process main app commands (APP_CMD_*)

		uint32_t flags = 0;
		detail::AndroidState* state = static_cast<detail::AndroidState*>(app->userData);
		AndroidWindow* window = _window_provider->get_android_window();

		// int32_t width, height;
		// width = ANativeWindow_getWidth(app->window);
		// height = ANativeWindow_getHeight(app->window);
		// LOGV("%i x %i\n", width, height);
		switch(command)
		{
			case APP_CMD_INPUT_CHANGED:
				break;

			// The window is being shown
			case APP_CMD_INIT_WINDOW:
				// LOGV("APP_CMD_INIT_WINDOW\n");
				state->surface_ready = true;
				_window_provider->set_native_window(app->window);
				break;

			// The window is being hidden or closed
			case APP_CMD_TERM_WINDOW:
				// LOGV("APP_CMD_TERM_WINDOW\n");
				_graphics_provider->detach_context(window);
				_graphics_provider->destroy_surface(window);
				_window_provider->set_native_window(nullptr);

				state->surface_ready = false;
				state->egl_valid = false;
				break;

			case APP_CMD_WINDOW_RESIZED:
				// LOGV("APP_CMD_WINDOW_RESIZED\n");
				break;

			case APP_CMD_WINDOW_REDRAW_NEEDED:
				// LOGV("APP_CMD_WINDOW_REDRAW_NEEDED\n");
				break;

			case APP_CMD_CONTENT_RECT_CHANGED:
				// LOGV("APP_CMD_CONTENT_RECT_CHANGED\n");
				break;

			// The app gains focus
			case APP_CMD_GAINED_FOCUS:
				// LOGV("APP_CMD_GAINED_FOCUS\n");
				// we have a valid window; keep the screen on
				flags = AWINDOW_FLAG_KEEP_SCREEN_ON;
				// AWINDOW_FLAG_FULLSCREEN does not hide the soft buttons.
				ANativeActivity_setWindowFlags(app->activity, flags, 0);

				state->has_focus = true;
				break;

			// The app lost focus
			case APP_CMD_LOST_FOCUS:
				// LOGV("APP_CMD_LOST_FOCUS\n");
				state->has_focus = false;
				break;

			case APP_CMD_CONFIG_CHANGED:
				// LOGV("APP_CMD_CONFIG_CHANGED\n");
				break;

			case APP_CMD_LOW_MEMORY:
				// LOGV("APP_CMD_LOW_MEMORY\n");
				break;

			case APP_CMD_START:
				// LOGV("APP_CMD_START\n");
				break;

			case APP_CMD_RESUME:
				// should re-create EGL context
				// LOGV("APP_CMD_RESUME\n");
				state->is_resumed = true;
				break;

			// The system has asked the application to save its state
			case APP_CMD_SAVE_STATE:
				// LOGV("APP_CMD_SAVE_STATE\n");
				break;

			case APP_CMD_PAUSE:
				// should destroy EGL context
				// LOGV("APP_CMD_PAUSE\n");
				state->is_resumed = false;
				break;

			case APP_CMD_STOP:
				// LOGV("APP_CMD_STOP\n");
				break;

			case APP_CMD_DESTROY:
				// LOGV("APP_CMD_DESTROY\n");
				break;
		}
	}

	static int32_t android_handle_input(struct android_app* app, AInputEvent* event)
	{
		// Fill this in with the function to process input events.  At this point
		// the event has already been pre-dispatched, and it will be finished upon
		// return.  Return 1 if you have handled the event, 0 for any default
		// dispatching.
		// int32_t event_type = AInputEvent_getType(event);


		// if (event_type == AINPUT_EVENT_TYPE_KEY)
		// {
		// 	int32_t event_action = AKeyEvent_getAction(event);
		// 	if (event_action == AKEY_EVENT_ACTION_UP)
		// 	{
		// 		ANativeActivity_finish(app->activity);
		// 		return 1;
		// 	}
		// }

		return 0;
	}


	// android_main runs in its own thread with its own event loop
	// for receiving input events.
	int backend_run_application(struct android_app* app)
	{
		// don't strip the native glue code
		app_dummy();

		detail::AndroidState state;
		app->userData = &state;
		_state = &state;

		// setup the application state
		state.app = app;
		state.vm = app->activity->vm;

		// install handlers for commands and input events
		app->onAppCmd = android_handle_command;
		app->onInputEvent = android_handle_input;

		gemini::core_startup();

		LOGV("__ANDROID_API__ is %i\n", __ANDROID_API__);
		LOGV("internalDataPath: %s\n", android::internal_data_path());
		LOGV("externalDataPath: %s\n", android::external_data_path());
		LOGV("obbPath: %s\n", android::obb_path());

		AndroidWindow* window = _window_provider->get_android_window();

		// If we want to bail early, we need to tell Android our
		// activity has finished.
		if (!state.is_running)
		{
			ANativeActivity_finish(app->activity);
		}

		bool processing_events = true;
		while(processing_events)
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
					LOGV("android state requested destroy!\n");
					// we're finished processing events
					processing_events = false;
					break;
				}
			}

			bool can_tick = (state.is_resumed && state.has_focus && state.surface_ready);
			if (can_tick)
			{
				// setup EGL objects
				//
				//
				if (!state.egl_valid)
				{
					// EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
					// guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
					// As soon as we picked a EGLConfig, we can safely reconfigure the
					// ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
					ANativeWindow_setBuffersGeometry(_window_provider->get_native_window(), 0, 0, window->visual);

					_graphics_provider->create_surface(window);

					// if the kernel hasn't started up yet; we must attach the context here
					// because the kernel expects a valid context for startup.
					if (!state.kernel_started)
					{
						_graphics_provider->attach_context(window);
					}
					state.egl_valid = true;
				}

				// start the kernel, if we haven't already
				if (!state.kernel_started && state.egl_valid)
				{
					kernel::startup();
					state.kernel_started = true;

					// We defer the tick for this frame because the system JUST
					// initialized. Likely, Android will kill our window and re-create it
					// so don't even bother attempting to render this frame.
					continue;
				}

				if (kernel::instance())
				{
					if (kernel::instance()->is_active())
					{
						kernel::instance()->tick();
					}
					// else
					// {
					// 	ANativeActivity_finish(app->activity);
					// }
				}
			}
		}

		// cleanup kernel memory
		if (state.kernel_started)
		{
			kernel::shutdown();
		}

		LOGV("exiting the android application\n");

		// shutdown the core
		gemini::core_shutdown();

		return 0;
	}

	void backend_shutdown()
	{
		AndroidWindow* window = _window_provider->get_android_window();
		_graphics_provider->destroy_context(window);

		MEMORY_DEALLOC(window->graphics_data, get_platform_allocator());
		_window_provider->destroy(window);

		assert(_graphics_provider != nullptr);
		_graphics_provider->shutdown(_window_provider);
		MEMORY_DELETE(_graphics_provider, get_platform_allocator());

		assert(_window_provider != nullptr);
		_window_provider->shutdown();
		MEMORY_DELETE(_window_provider, get_platform_allocator());
		_window_provider = nullptr;
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
				return Result::failure("The only supported rendering backend is OpenGL ES 2");
			}

			return Result::success();
		}

		void shutdown()
		{

		}

		void dispatch_events()
		{

		}

		NativeWindow* create(const Parameters& window_parameters)
		{
			return _window_provider->get_android_window();
		}

		void destroy(NativeWindow* window)
		{
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
	} // namespace window

	namespace android
	{
		const char* internal_data_path()
		{
			return _state->app->activity->internalDataPath;
		}

		const char* external_data_path()
		{
			return _state->app->activity->externalDataPath;
		}

		const char* obb_path()
		{
			return _state->app->activity->obbPath;
		}
	} // namespace android


	size_t system_pagesize()
	{
		return sysconf(_SC_PAGESIZE);
	}

	size_t system_processor_count()
	{
		return sysconf(_SC_NPROCESSORS_ONLN);
	}
} // namespace platform

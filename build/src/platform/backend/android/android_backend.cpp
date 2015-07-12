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
#include "android_backend.h"
#include "graphics_provider.h"
#include "../../window/android/android_window_provider.h"

#include <android/log.h>

#include <android/native_window_jni.h> // for ANativeWindow_fromSurface
#include <jni.h>

#if defined(PLATFORM_EGL_SUPPORT)
	#include "../../graphics/egl/egl_graphics_provider.h"
#endif

using namespace platform::window;

#define NATIVE_LOG( fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "gemini", fmt, ##__VA_ARGS__)

const char JNI_CLASS_PATH[] = "net/arcfusion/gemini/gemini_activity";

static ANativeWindow* _window = nullptr;

void native_start(JNIEnv* env, jclass the_class)
{
	NATIVE_LOG("native_start called\n");
}

void native_stop(JNIEnv* env, jclass the_class)
{
	NATIVE_LOG("native_stop called\n");
}

void native_pause(JNIEnv* env, jclass the_class)
{
	NATIVE_LOG("native_pause called\n");
}

void native_resume(JNIEnv* env, jclass the_class)
{
	NATIVE_LOG("native_resume called\n");
}

void native_destroy(JNIEnv* env, jclass the_class)
{
	NATIVE_LOG("native_destroy called\n");
}

void native_set_surface(JNIEnv* env, jclass the_class, jobject surface)
{
	NATIVE_LOG("native_set_surface called\n");
	if (surface)
	{
		_window = ANativeWindow_fromSurface(env, surface);
		NATIVE_LOG("fetch window from surface: %p\n", _window);
	}
	else
	{
		NATIVE_LOG("releasing the surface\n");
		ANativeWindow_release(_window);
	}
}

static JNINativeMethod method_table[] = {
		{"native_start", "()V", (void*)native_start},
		{"native_stop", "()V", (void*)native_stop},
		{"native_pause", "()V", (void*)native_pause},
		{"native_resume", "()V", (void*)native_resume},
		{"native_destroy", "()V", (void*)native_destroy},
		{"native_set_surface", "(Landroid/view/Surface;)V", (void*)native_set_surface},
};

// The VM calls JNI_OnLoad when the native library is loaded (for example, through 
// System.loadLibrary).

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	static bool _jni_loaded = false;
	if (_jni_loaded)
		return JNI_VERSION_1_6;

	NATIVE_LOG("JNI_OnLoad called...\n");

	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
	{
		NATIVE_LOG("Unable to setup environment\n");
		return -1;
	}
	else
	{
		NATIVE_LOG("Loading classes from '%s'\n", JNI_CLASS_PATH);
		// this path must match the path to the .java class
		jclass cl = env->FindClass(JNI_CLASS_PATH);
		if (!cl)
		{
			NATIVE_LOG("Error registering JNI functions\n");
			return -1;
		}
		else
		{
			env->RegisterNatives(cl, method_table, sizeof(method_table) / sizeof(method_table[0]));
			env->DeleteLocalRef(cl);
		}
	}

	_jni_loaded = true;
	return JNI_VERSION_1_6;
} // JNI_OnLoad


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
			return _window;
		}
	} // namespace window
} // namespace platform
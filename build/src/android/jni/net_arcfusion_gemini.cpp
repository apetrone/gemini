// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#include <android/log.h>
#include <jni.h>
#include "memory.hpp"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "kernel_android.hpp"
#include "filesystem.hpp"

const char JNI_CLASS_PATH[] = "net/arcfusion/gemini/Lynx";

#define NATIVE_LOG( fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "gemini", fmt, ##__VA_ARGS__)

namespace lynx
{
	AndroidKernel * _kernel = 0;

	void gemini_startup(JNIEnv * env, jclass the_class, jobject jasset_manager)
	{
		// this should not exist during startup
		if ( _kernel == 0 )
		{
			memory::startup();
			NATIVE_LOG( "gemini_startup called\n" );

			_kernel = CREATE(AndroidKernel);

			// acquire the asset manager pointer
			AAssetManager * asset_manager = AAssetManager_fromJava(env, jasset_manager);

			fs::set_asset_manager(asset_manager);
		}
		else
		{
			NATIVE_LOG( "kernel instance already active - not creating another...\n" );
		}
	} // gemini_startup


	void gemini_shutdown(JNIEnv * env, jclass the_class)
	{
		NATIVE_LOG( "gemini_shutdown called\n" );

		// shutdown the kernel
		kernel::shutdown();

		// destroy our instance
		DESTROY(AndroidKernel, _kernel);

		memory::shutdown();
	} // gemini_shutdown

	void gemini_tick(JNIEnv * env, jclass the_class)
	{
		kernel::tick();
	} // gemini_tick

	void gemini_surface_changed(JNIEnv * env, jclass the_class, jint width, jint height)
	{
		NATIVE_LOG( "kernel %p surface changed to %i x %i\n", _kernel, width, height );
		if ( _kernel )
		{
			// set the new surface dimensions
			_kernel->on_surface_changed(width, height);

			// if we haven't run startup; do so now that the viewport is ready and gl context is active
			if ( !_kernel->is_initialized() )
			{
				NATIVE_LOG( "performing kernel startup...\n" );
				if ( kernel::startup( _kernel ) != kernel::NoError )
				{
					NATIVE_LOG( "kernel startup failed!" );
				}
			}
		}
	} // gemini_surface_changed

	void gemini_set_display_density( JNIEnv * env, jclass the_class, jfloat density )
	{
		if ( _kernel )
		{
			_kernel->set_display_density( density );
		}
	} // gemini_set_display_density


	void gemini_set_status_bar_height( JNIEnv * env, jclass the_class, jint height )
	{
		if ( _kernel )
		{
			_kernel->set_status_bar_height( height );
		}
	} // gemini_set_status_bar_height

}; // namespace lynx


static JNINativeMethod method_table[] = {
		{"gemini_startup", "(Landroid/content/res/AssetManager;)V", (void*)lynx::gemini_startup},
		{"gemini_shutdown", "()V", (void*)lynx::gemini_shutdown},
		{"gemini_tick", "()V", (void*)lynx::gemini_tick},
		{"gemini_surface_changed", "(II)V", (void*)lynx::gemini_surface_changed},
		{"gemini_set_display_density", "(F)V", (void*)lynx::gemini_set_display_density},
		{"gemini_set_status_bar_height", "(I)V", (void*)lynx::gemini_set_status_bar_height},
};

jint JNI_OnLoad( JavaVM * vm, void * reserved)
{
	JNIEnv * env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
	{
		NATIVE_LOG( "Unable to setup environment\n" );
		return -1;
	}
	else
	{
		NATIVE_LOG( "Loading classes from '%s'\n", JNI_CLASS_PATH );
		// this path must match the path to the .java class
		jclass cl = env->FindClass( JNI_CLASS_PATH );
		if (!cl)
		{
			NATIVE_LOG( "Error loading lynx\n" );
			return -1;
		}
		else
		{
			env->RegisterNatives(cl, method_table, sizeof(method_table) / sizeof(method_table[0]));
			env->DeleteLocalRef(cl);
		}
	}

	return JNI_VERSION_1_6;
} // JNI_OnLoad

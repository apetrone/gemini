#include <android/log.h>
#include <jni.h>
#include "memory.hpp"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "kernel_android.hpp"
#include "filesystem.hpp"

const char JNI_CLASS_PATH[] = "net/arcfusion/gemini/Lynx";

#define NATIVE_LOG( fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "lynx", fmt, ##__VA_ARGS__)

namespace lynx
{
	AndroidKernel * _kernel = 0;

	void test(JNIEnv * env, jclass the_class)
	{
		NATIVE_LOG( "test called!\n" );
	} // test


	void gemini_startup(JNIEnv * env, jclass the_class, jobject jasset_manager)
	{
		NATIVE_LOG( "gemini_startup called\n" );

		memory::startup();

		_kernel = CREATE(AndroidKernel);

		// acquire the asset manager pointer
		AAssetManager * asset_manager = AAssetManager_fromJava(env, jasset_manager);

		fs::set_asset_manager(asset_manager);

		if ( kernel::startup( _kernel, "TestUniversal" ) != kernel::NoError )
		{
			NATIVE_LOG( "kernel startup failed!" );
		}

#if 0 // testing the asset manager / paths
		const char * filename = 0;
		AAssetDir* assetdir = AAssetManager_openDir(asset_manager, "");

		do
		{
			filename = AAssetDir_getNextFileName(assetdir);
			NATIVE_LOG( "-> %s\n", filename );
		} while( filename );


		if ( fs::file_exists("file.txt") )
		{
			NATIVE_LOG( "file.txt EXISTS\n" );
		}
		else
		{
			NATIVE_LOG( "file.txt DOES NOT EXIST!\n" );
		}
#endif
	} // gemini_startup


	void gemini_shutdown(JNIEnv * env, jclass the_class)
	{
		NATIVE_LOG( "gemini_shutdown called\n" );

		_kernel->shutdown();
		DESTROY(AndroidKernel, _kernel);

		memory::shutdown();
	} // gemini_shutdown

	void gemini_tick(JNIEnv * env, jclass the_class)
	{
		kernel::tick();
	} // gemini_tick

}; // namespace lynx


static JNINativeMethod method_table[] = {
		{"test", "()V", (void*)lynx::test},
		{"gemini_startup", "(Landroid/content/res/AssetManager;)V", (void*)lynx::gemini_startup},
		{"gemini_shutdown", "()V", (void*)lynx::gemini_shutdown},
		{"gemini_tick", "()V", (void*)lynx::gemini_tick}
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

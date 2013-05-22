#include <android/log.h>
#include <jni.h>

const char JNI_CLASS_PATH[] = "net/arcfusion/gemini/Lynx";

#define NATIVE_LOG( fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "lynx", fmt, ##__VA_ARGS__)

namespace lynx
{
	void test(JNIEnv * env, jclass the_class)
	{
		NATIVE_LOG( "test called!\n" );
	} // test
}; // namespace lynx


static JNINativeMethod method_table[] = {
		{"test", "()V", (void*)lynx::test}
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

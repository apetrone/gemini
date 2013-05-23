package net.arcfusion.gemini;

import android.content.res.AssetManager;

public class Lynx
{
	//
	// Native declarations
	public native static void test();
	
	public native static void gemini_startup( AssetManager assetManager );
	public native static void gemini_shutdown();
	
	
	public native static void gemini_tick();
	
	public static AssetManager asset_manager = null;
	
	static {
		// this path corresponds to the name of the native library
		System.loadLibrary("net_arcfusion_gemini_lynx");
	}
}

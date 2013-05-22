package net.arcfusion.gemini;

public class Lynx
{
	//
	// Native declarations
	public native static void test();
	
	static {
		// this path corresponds to the name of the native library
		System.loadLibrary("net_arcfusion_gemini_lynx");
	}
}

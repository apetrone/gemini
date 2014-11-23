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
package net.arcfusion.gemini;

import android.content.res.AssetManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;

public class Lynx
{
	public static final String TAG = "Lynx";
	
	//
	// Native declarations
	public native static void gemini_startup( AssetManager assetManager );
	public native static void gemini_shutdown();	
	public native static void gemini_tick();
	public native static void gemini_surface_changed( int width, int height );
	public native static void gemini_set_display_density( float density );	
	public native static void gemini_set_status_bar_height( int height );
	
	
	public static void set_content_view( View view )
	{
		Lynx.content_view = view;
	}
	
	public static void surface_changed( int width, int height )
	{
		gemini_surface_changed( width, height );

		if ( Lynx.content_view != null )
		{
	        DisplayMetrics metrics = Lynx.content_view.getResources().getDisplayMetrics();
	        Log.v(TAG, "Display density DPI: " + metrics.densityDpi );
	
	        int status_bar_height = (int) Math.ceil(metrics.density * 25);
	        Log.v(TAG, "status bar height: " + status_bar_height );
	        
	        gemini_set_display_density( metrics.density );
	        gemini_set_status_bar_height( status_bar_height );
		}
	}
	
	
	public static AssetManager asset_manager = null;
	public static View content_view = null;
	
	static {
		// this path corresponds to the name of the native library
		System.loadLibrary("net_arcfusion_gemini_lynx");
	}
}

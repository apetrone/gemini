// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

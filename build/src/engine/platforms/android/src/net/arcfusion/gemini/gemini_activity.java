// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone
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

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Window;

public class gemini_activity extends Activity {
	public static final String TAG = "gemini_activity";
	gemini_gl2_view content_view;
	
    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        
        Lynx.asset_manager = getAssets();
        content_view = new gemini_gl2_view(getApplication(), true, 24, 0);
        setContentView(content_view);
        
        Lynx.set_content_view(content_view);
    }

    @Override protected void onPause() {
        super.onPause();
        Log.v(TAG, "onPause");
        content_view.onPause();
    }
 

    @Override protected void onResume() {
        super.onResume();
        Log.v(TAG, "onResume");

        DisplayMetrics metrics = getResources().getDisplayMetrics();
        Log.v(TAG, "Display density DPI: " + metrics.densityDpi );

        int status_bar_height = (int) Math.ceil(metrics.density * 25);
        Log.v(TAG, "status bar height: " + status_bar_height );        

        content_view.onResume();
    }
    
    @Override protected void onDestroy() {
    	// called when removed from active task list
    	Lynx.gemini_shutdown();
    	super.onDestroy();
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        //here you can handle orientation change
    }    
}

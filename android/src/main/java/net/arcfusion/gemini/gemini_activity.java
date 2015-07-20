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
import android.app.NativeActivity;

// import android.content.pm.ActivityInfo;
// import android.content.res.Configuration;
// import android.graphics.Rect;
// import android.os.Bundle;
// import android.util.DisplayMetrics;
import android.util.Log;
// import android.view.Window;
import java.io.File;
import android.content.Context;
import android.app.Application;
import android.os.Environment;

public class gemini_activity extends NativeActivity
{
	public static final String TAG = "gemini_activity";


	public void java_call()
	{
		Log.v(TAG, "java call invoked!");
        Context context = getApplicationContext();

        // starting with KITKAT, we don't need permissions to write to these directories
        
        // e.g. /storage/emulated/0/Android/data/<package-name>/cache
        Log.v(TAG, String.format("Context.getExternalCacheDir: %s", context.getExternalCacheDir().getAbsolutePath()));

        // e.g. /storage/emulated/0/Android/data/<package-name>/files
        Log.v(TAG, String.format("Context.getExternalFilesDir: %s", context.getExternalFilesDir(null).getAbsolutePath()));

        // e.g. /storage/emulated/0/Android/obb/<package-name>
        Log.v(TAG, String.format("Context.getObbDir: %s", context.getObbDir().getAbsolutePath()));
	}

    static
    {
        Log.v(TAG, "gemini_activity static init!");

        // e.g. "/data"
        Log.v(TAG, String.format("Environment.getDataDirectory: %s", Environment.getDataDirectory().getAbsolutePath()));

        // e.g. "/cache"
        Log.v(TAG, String.format("Environment.getDownloadCacheDirectory: %s", Environment.getDownloadCacheDirectory().getAbsolutePath()));

        // e.g. "/storage/emulated/0"
        Log.v(TAG, String.format("Environment.getExternalStorageDirectory: %s", Environment.getExternalStorageDirectory().getAbsolutePath()));

        // e.g.
        File externalStorage = Environment.getExternalStorageDirectory();
        Log.v(TAG, String.format("Environment.getExternalStorageState: %s", Environment.getExternalStorageState(externalStorage)));
    }

    // OBB storage is: <shared-storage>/Android/obb/<package-name>

    // static 
    // {
    //     try
    //     {
    //         Log.v(TAG, "loading libcore.so");
    //         System.loadLibrary("core");
    //         Log.v(TAG, "loading libplatform.so");
    //         System.loadLibrary("platform");
    //         Log.v(TAG, "loading librenderer.so");
    //         System.loadLibrary("renderer");
    //         Log.v(TAG, "loading libruntime.so");            
    //         System.loadLibrary("runtime");
    //     }
    //     catch (UnsatisfiedLinkError exception)
    //     {
    //         System.err.println("One or more native code library failed to load!\n" + exception);
    //         System.exit(1);
    //     }
    // }
}
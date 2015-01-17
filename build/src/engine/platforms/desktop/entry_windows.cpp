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
#include <iostream>
#include <platform/platform.h>
#include "kernel_desktop.h"
 
#include <windows.h>

using namespace gemini;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR commandline, int show)
{
	platform::startup();
	// TODO: parse the LPSTR commandline
	char *argv[] = { "-game", "C:\\Users\\apetrone\\dev\\games" };
	kernel::parse_commandline(2, argv);

	// structure this so the desktop_kernel goes out of scope before
	// we call memory::shutdown
	kernel::Error error = kernel::NoError;
	{
		DesktopKernel desktop_kernel(2, argv);

		// set title bar height, debugdraw uses this as an offset for text.
		desktop_kernel.parameters().titlebar_height = GetSystemMetrics(SM_CYCAPTION);

		error = kernel::main( &desktop_kernel, "TestUniversal" );
	}
	
	platform::shutdown();
	return (int)error;
}
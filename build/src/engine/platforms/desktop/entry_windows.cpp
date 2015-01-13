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
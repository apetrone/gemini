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
#pragma once


#if _WIN32
	#include <limits.h>
	#include <windows.h>
	#include <gl/gl.h>
	#include <glext.h>
	#include <wglext.h>
	#pragma comment( lib, "opengl32.lib" )
#elif LINUX
	#include <stdint.h>
	#include <GL/gl.h>
	#include <GL/glx.h>
	//#include <glxext.h>
#elif __APPLE__
	#include <stdint.h>
	#include <TargetConditionals.h>

	#if TARGET_OS_IPHONE
		#include <OpenGLES/ES2/gl.h>
		#include <OpenGLES/ES2/glext.h>
	#elif TARGET_OS_MAC
		#include <OpenGL/gl3.h>
		#include <OpenGL/gl3ext.h>
		// legacy GL
//		#include <OpenGL/gl.h>
//		#include <OpenGL/glext.h>
	#endif
#endif
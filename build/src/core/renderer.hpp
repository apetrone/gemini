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

#include "typedefs.h"
#include "memory.hpp"
#include "memorystream.hpp"



namespace renderer
{
	enum DriverType
	{
		Default, // pick one.
		OpenGL,
		GLESv2,
		GLESv3
	}; // DriverType
	
	
	enum DriverCommandType
	{
		DC_SHADER,
		DC_UNIFORMMATRIX4,
		DC_UNIFORM1i,
		DC_UNIFORM3f,
		DC_UNIFORM4f,
		DC_UNIFORM_SAMPLER_2D,
		DC_UNIFORM_SAMPLER_CUBE,
		
		DC_CLEAR,
		DC_CLEARCOLOR,
		DC_CLEARDEPTH,
		DC_VIEWPORT,
		
		DC_DRAWCALL,
		DC_SCISSOR,
		
		DC_MAX
	}; // DriverCommandType
	
	// returns 0 on failure, 1 on success
	int startup( DriverType driver );
	void shutdown();

	
	//
	// IRenderDriver
	// The render driver acts as a command processor. The implementation details are up to the driver
	// which make this a nice abstraction layer.
	class IRenderDriver
	{
	public:
		virtual ~IRenderDriver() {}
		virtual const char * description() = 0;
		
		// these commands are called with the command and current memory stream
		virtual void run_command( DriverCommandType command, MemoryStream & stream ) = 0;
		virtual void post_command( DriverCommandType command, MemoryStream & stream ) = 0;

	}; // IRenderDriver
	typedef IRenderDriver * (*RenderDriverCreator)();
	
	
	IRenderDriver * driver();

	
}; // namespace renderer
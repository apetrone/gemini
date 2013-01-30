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
#include "kernel.hpp"
#include <stdio.h>

class HelloWorld : public kernel::IApplication,
	kernel::IEventListener<kernel::TouchEvent>

{
public:
	DECLARE_APPLICATION( HelloWorld );

	virtual void event( kernel::TouchEvent & event )
	{
		fprintf( stdout, "Touch Event Happening!\n" );
	}
	
	virtual int config( kernel::Params & params )
	{
		
		kernel::subscribe_event<kernel::TouchEvent>( this );
		
		return kernel::NoWindow;
	}

	virtual int startup( kernel::Params & params )
	{
		printf( "Hello World!\n" );
		return kernel::NoWindow;
	}

	virtual void tick( kernel::Params & params )
	{
	}
};

IMPLEMENT_APPLICATION( HelloWorld );
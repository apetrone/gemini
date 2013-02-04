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
#include "memorystream.hpp"
#include "log.h"

class TestMemoryStream : public kernel::IApplication
{
public:
	DECLARE_APPLICATION( TestMemoryStream );

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		return kernel::NoWindow;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		MemoryStream s;
		char data[16] = {0};
		s.init( data, 16 );
		s.write( "abc", 3 );
		
		char out[ 4 ];
		s.rewind();
		s.read( out, 3 );
		LOGV( "out: '%s', %i\n", out, s.offset_pointer() );
		
		
		// okay, so the base functions read and write arbitrary data streams properly.
		// how about these templatized functions?
		int valueA = 1273;
		int valueB = 0;		
		s.rewind();
		s.write( valueA );
		s.rewind();
		s.read( &valueB );
		LOGV( "valueB: %i\n", valueB );
		
		s.rewind();
		// hmm, what about floats?
		float z = 884.2123;
		float v = 0;
		s.write( z );
		s.rewind();
		s.read( &v );
		LOGV( "v: %g\n", v );

		s.rewind();
		// classy, but can you write a pointer?
		int m = 12345;
		int *x = &m;
		int *p = 0;
		s.write( x );
		s.rewind();
		s.read( &p );
		LOGV( "x = '%p', p = '%p'\n", x, p );
		
		return kernel::NoWindow;
	}

	virtual void tick( kernel::Params & params )
	{
	}

	virtual void shutdown()
	{
	}
};

IMPLEMENT_APPLICATION( TestMemoryStream );
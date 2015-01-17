// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <stdio.h>

#include <platform/config.h>
#include <slim/xlog.h>

#include "kernel.h"
#include <core/datastream.h>

using namespace gemini::util;
class TestMemoryStream : public kernel::IApplication
{
public:
	DECLARE_APPLICATION( TestMemoryStream );

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		return kernel::Application_NoWindow;
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
		LOGV( "out: '%s', %i\n", out, s.current_offset() );
		
		
		// okay, so the base functions read and write arbitrary data streams properly.
		// how about these templatized functions?
		int valueA = 1273;
		int valueB = 0;		
		s.rewind();
		s.write( valueA );
		s.rewind();
		s.read( valueB );
		LOGV( "valueB: %i\n", valueB );
		
		s.rewind();
		// hmm, what about floats?
		float z = 884.2123;
		float v = 0;
		s.write( z );
		s.rewind();
		s.read( v );
		LOGV( "v: %g\n", v );

		s.rewind();
		// classy, but can you write a pointer?
		int m = 12345;
		int *x = &m;
		int *p = 0;
		s.write( x );
		s.rewind();
		s.read( p );
		LOGV( "x = '%p', p = '%p'\n", x, p );
		
		return kernel::Application_NoWindow;
	}

	virtual void step( kernel::Params & params )
	{
	}

	virtual void tick( kernel::Params & params )
	{
	}

	virtual void shutdown( kernel::Params & params )
	{
	}
};

IMPLEMENT_APPLICATION( TestMemoryStream );
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
#include <slim/xlog.h>


#include "keyvalues.hpp"

class TestClasses : public kernel::IApplication
{
public:
	DECLARE_APPLICATION( TestClasses );

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{		
		return kernel::Application_NoWindow;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		KeyValues k;
		k.set( "size", 30 );
		
		int v = k.get( "size", 0 );
		LOGV( "v = %i\n", v );
		
		
		k.set( "test", 2.32f );
		
		float z = k.get( "test", 0.0f );
		LOGV( "f = %g\n", z );

		glm::vec3 t;
		k.set( "vec", glm::vec3(1.3f, 2.2f, 3.1f) );
		
		t = k.get("vec", glm::vec3() );
		LOGV( "vec3: %g %g %g\n", t.x, t.y, t.z );
				
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

IMPLEMENT_APPLICATION( TestClasses );
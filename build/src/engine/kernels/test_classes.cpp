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
#include "keyvalues.h"

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
#if 0
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
#endif

		
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
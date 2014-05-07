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

#include "hashtable.hpp"
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
		ChainedHashTable<int, unsigned int> key_map;

		key_map.set(0, 32);
		key_map.set(1, 64);
		key_map.set(2, 128);
		key_map.set(3, 256);
		
		key_map.set(1, 256);
		key_map.set(2, 512);
		key_map.set(3, 1024);


		unsigned int value = key_map.get(0, UINT_MAX);
		LOGV("value: %i\n", value);
		
		value = key_map.get(1, UINT_MAX);
		LOGV("value: %i\n", value);
		
		value = key_map.get(2, UINT_MAX);
		LOGV("value: %i\n", value);
		
		value = key_map.get(3, UINT_MAX);
		LOGV("value: %i\n", value);
		
		
		ChainedHashTable<char*, unsigned int> string_map;
		
		char name[] = "two";
		char* const ref = "three";
		
		string_map.set("one", 1);
		string_map.set("two", 2);
		string_map.set("three", 3);
		
		value = string_map.get("one", UINT_MAX);
		LOGV("string_map: %i\n", value);

		value = string_map.get(name, UINT_MAX);
		LOGV("string_map: %i\n", value);
		
		value = string_map.get(ref, UINT_MAX);
		LOGV("string_map: %i\n", value);
		
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
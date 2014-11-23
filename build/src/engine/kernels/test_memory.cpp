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
#include <stdio.h>
#include <vector>
#include <map>

#include <gemini/typedefs.h>
#include <gemini/mem.h>
#include <slim/xlog.h>

#include "kernel.h"
#include <gemini/util/fixedarray.h>

struct Test
{
	size_t def;
	
	Test()
	{
		printf( "Test()\n" );
	}
	
	~Test()
	{
		printf( "~Test()\n" );
	}
};

struct BaseObject
{
	int type;
	
	BaseObject() : type(0)
	{
		LOGV("BaseObject\n");
	}

	virtual ~BaseObject()
	{
		LOGV("~BaseObject\n");
	}
};


struct DerivedObject : public BaseObject
{
	int test;
	
	DerivedObject() : test(0)
	{
		LOGV("DerivedObject\n");
	}
	
	~DerivedObject()
	{
		LOGV("~DerivedObject\n");
	}
};

struct PooledObject
{
	int test_data;
	
	void * operator new ( std::size_t count )
	{
		void * ptr = ALLOC( count );
		LOGV( "alloc size: %i bytes -> %p\n", count, ptr );
		return ptr;
	}
	
	void * operator new ( std::size_t count, void * ptr )
	{
		LOGV( "alloc size: %i bytes -> %p\n", count, ptr );
		return ptr;
	}
	
	void operator delete( void * ptr )
	{
		LOGV( "dealloc pointer: %p\n", ptr );
		DEALLOC( ptr );
	}
};


struct DerivedPoolObject : public PooledObject
{
	float delta;
	float x, y, z;
	
	
};

class TestMemory : public kernel::IApplication
{
public:
	DECLARE_APPLICATION( TestMemory );

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		return kernel::Application_NoWindow;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
#if 0
		printf( "Memory Test: \n" );
		Test * a = CREATE(Test);
		
		std::vector<int, GeminiAllocator<int> > int_vector;
		int_vector.push_back( 30 );
		int_vector.push_back( 16 );
		int_vector.push_back( 31 );
		
		typedef std::map<int, int, std::less<int>, GeminiAllocator<int> > MyMap;
		MyMap int_map;
		
		int_map.insert( MyMap::value_type( 30, 17 ) );
		int_map.insert( MyMap::value_type( 16, 100) );
		int_map.insert( MyMap::value_type( 300, 2 ) );

		// Test FixedArray - which should cause derived destructors
		// to be called as well as the BaseObject's destructors.
		FixedArray<BaseObject> objects;
		objects.allocate(3);
		for( size_t i = 0; i < 3; ++i )
		{
			objects[i] = CREATE(DerivedObject);
			objects[i]->type = i+4;
		}
		objects.purge();#

		// added z-modifer to satisfy Xcode, C99 addition, we'll see who doesn't support it :)
		printf( "totalAllocations: %zu, totalBytes: %zu\n", memory::allocator().total_allocations(), memory::allocator().total_bytes() );
		printf( "activeAllocations: %zu, activeBytes: %zu\n", memory::allocator().active_allocations(), memory::allocator().active_bytes() );

		DESTROY(Test, a);
		printf( "activeAllocations: %zu, activeBytes: %zu\n", memory::allocator().active_allocations(), memory::allocator().active_bytes() );
#endif
		
		PooledObject * p = new DerivedPoolObject();
		
		
		delete p;

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

IMPLEMENT_APPLICATION( TestMemory );
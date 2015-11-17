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
#include <vector>
#include <map>

#include <core/typedefs.h>
#include <platform/mem.h>
#include <slim/xlog.h>

#include "kernel.h"
#include <core/fixedarray.h>

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

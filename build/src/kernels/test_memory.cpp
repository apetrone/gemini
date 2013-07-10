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
#include "typedefs.h"
#include "memory.hpp"

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

#include <vector>


#include <map>



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

		
		
		// added z-modifer to satisfy Xcode, C99 addition, we'll see who doesn't support it :)
		printf( "totalAllocations: %zu, totalBytes: %zu\n", memory::allocator().total_allocations(), memory::allocator().total_bytes() );
		printf( "activeAllocations: %zu, activeBytes: %zu\n", memory::allocator().active_allocations(), memory::allocator().active_bytes() );
		
		DESTROY(Test, a);
		printf( "activeAllocations: %zu, activeBytes: %zu\n", memory::allocator().active_allocations(), memory::allocator().active_bytes() );
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
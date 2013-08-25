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

#include "typedefs.h"
#include "keyvalues.hpp"

#include <slim/xlog.h>
#include <slim/xstr.h>


class IntPolicy : public PolicyBase
{
	int value;
	
public:
	virtual DataType type() { return DATA_INT; }

	virtual void destroy( const void * data )
	{
		
	}
	
	virtual void create( const void * data )
	{
		update( data );
	}
	
	virtual void update( const void * type )
	{
		value = *((int*)type);
	}
	
	virtual void get( void * target )
	{
		int * p = (int*)target;
		*p = value;
	}
};
IMPLEMENT_POLICY(IntPolicy);



policy_creator KeyValues::policy_for_type( DataType type )
{
	LOGV( "policy type: %i\n", type );

	policy_creator policy_table[] =
	{
		0,
		MAKE_POLICY(IntPolicy)
	};
	
	return policy_table[ type ];
}

KeyValues::KeyValues()
{
	this->policy = 0;
}

KeyValues::~KeyValues()
{
	if ( this->policy )
	{
		DESTROY(PolicyBase, this->policy);
		this->policy = 0;
	}
}





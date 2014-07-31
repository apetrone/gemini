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

#include "keyvalues.h"
#include <gemini/typedefs.h>

#include <slim/xlog.h>
#include <slim/xstr.h>

#include <gemini/mathlib.h>

template <class Type>
class PODPolicy : public PolicyBase
{
	Type value;
	
public:
	virtual DataType type() { return keyvalues_typemap<Type>::get_type(); }
	
	virtual void destroy()
	{
		
	}
	
	virtual void create( const void * data )
	{
		update( data );
	}
	
	virtual void update( const void * type )
	{
		value = *((Type*)type);
	}
	
	virtual void get( void * target )
	{
		Type * p = (Type*)target;
		*p = value;
	}
};


IMPLEMENT_POLICY(PODPolicy<int>, IntPolicy);
IMPLEMENT_POLICY(PODPolicy<float>, FloatPolicy);





class Vec3Policy : public PolicyBase
{
	glm::vec3 * vector;
	
public:
	virtual DataType type() { return keyvalues_typemap<glm::vec3>::get_type(); }
	
	virtual void destroy()
	{
		using glm::vec3;
		DESTROY( vec3, vector );
	}
	
	virtual void create( const void * data )
	{
		vector = CREATE(glm::vec3);
		
		update( data );
	}
	
	virtual void update( const void * type )
	{
		*vector = *((glm::vec3*)type);	
	}
	
	virtual void get( void * target )
	{
		glm::vec3 * p = (glm::vec3*)target;
		*p = *vector;
	}
};
IMPLEMENT_POLICY(Vec3Policy, Vec3Policy);



policy_creator KeyValues::policy_for_type( DataType type )
{
	LOGV( "policy type: %i\n", type );
	
	assert( type >= 0 && type < DATA_MAX );

	policy_creator policy_table[DATA_MAX] =
	{
		0,
		MAKE_POLICY(IntPolicy),
		MAKE_POLICY(FloatPolicy),
		0,
		MAKE_POLICY(Vec3Policy),
		0,
		
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
		this->policy->destroy();
		
		DESTROY(PolicyBase, this->policy);
		this->policy = 0;
	}
}





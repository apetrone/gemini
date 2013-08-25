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
#pragma once


#include "color.hpp"
enum DataType
{
	DATA_INVALID = 0,
	DATA_INT,
	DATA_FLOAT,
	DATA_BOOL,
	DATA_STRING,
	DATA_POINTER,
	DATA_COLOR,
};





class PolicyBase
{
public:
	virtual ~PolicyBase() {}
	virtual DataType type() = 0;
	virtual void destroy( const void * data ) = 0;
	virtual void create( const void * data ) = 0;
	virtual void update( const void * type ) = 0;
	virtual void get( void * target ) = 0;

};

typedef PolicyBase * (*policy_creator)();

template <class Type>
struct keyvalues_typemap
{
	static DataType get_type()
	{
		return DATA_INVALID;
	}
};

#define DECLARE_POLICY_TYPE( type, data_type )\
	template <>\
	struct keyvalues_typemap<type>\
	{\
		static DataType get_type()\
		{\
			return data_type;\
		}\
	}

#define IMPLEMENT_POLICY( classname )\
	PolicyBase * classname##_create()\
	{\
		return CREATE(classname);\
	}

#define MAKE_POLICY( classname )\
	classname##_create



DECLARE_POLICY_TYPE(int, DATA_INT);


struct KeyValues
{
	char * name;
	
	KeyValues * next;
	KeyValues * prev;
	KeyValues * parent;
	KeyValues * children;

	KeyValues();
	~KeyValues();
	
	PolicyBase * policy;
	
	static policy_creator policy_for_type( DataType type );

	template <class Type>
	void set( const char * key, const Type & value )
	{
		DataType target_type = keyvalues_typemap<Type>::get_type();
		if ( this->policy && this->policy->type() != target_type )
		{
			DESTROY(PolicyBase, this->policy);
		}
		
		policy = KeyValues::policy_for_type(target_type)();
		policy->create( &value );
	}

	template <class Type>
	Type get( const char * key, const Type & default_value )
	{
		DataType target_type = keyvalues_typemap<Type>::get_type();
		int value = default_value;
		if ( this->policy && this->policy->type() == target_type )
		{
			policy->get( &value );
		}
		
		return value;
	}
}; // KeyValues

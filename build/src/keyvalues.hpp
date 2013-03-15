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



struct KeyValues
{
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


	char * name;

	union
	{
		int int_value;
		float float_value;
		void * pointer_value;
		unsigned char bytes[4];
	};
	
	KeyValues * next;
	KeyValues * prev;
	KeyValues * parent;
	KeyValues * children;

	KeyValues();
	~KeyValues();
	
	


	template <class Type>
	void set( const char * key, const Type & value );

	template <class Type>
	Type & get( const char * key, Type & default_value );
}; // KeyValues

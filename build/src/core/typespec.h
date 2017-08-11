// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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
#pragma once

#include <core/typedefs.h>
#include <stddef.h>
#include <core/str.h> // for STRING_HASH32

#include <type_traits>

#define TYPESPEC_REGISTER_NAME(C)\
	template <>\
	const char* TypeSpecName< C >::value = #C

#define TYPESPEC_IDENTIFIER(x)	STRING_HASH32(x)
#define TYPESPEC_REGISTER_IDENTIFIER(C)\
	template <>\
	size_t TypeSpecIdentifier< C >::value = TYPESPEC_IDENTIFIER(#C)

#define TYPESPEC_REGISTER_SIZE(C)\
	template <>\
	unsigned int TypeSpecSize< C >::value = sizeof(C)

#define TYPESPEC_DECLARE_CLASS(C, B)\
	public:\
	virtual const TypeSpecInfo* typespec() const override\
	{\
		return typespec_make_info< C >();\
	}

#define TYPESPEC_DECLARE_CLASS_NOBASE(C)\
	public:\
	virtual const TypeSpecInfo* typespec() const\
	{\
		return typespec_make_info< C >();\
	}

// Used to register classes.
// This needs to handle fully qualified class names (e.g. gui::MenuBar)
#define TYPESPEC_REGISTER_CLASS(C)\
	TYPESPEC_REGISTER_NAME(C);\
	TYPESPEC_REGISTER_IDENTIFIER(C);\
	TYPESPEC_REGISTER_SIZE(C)

// Used to register POD types
#define TYPESPEC_REGISTER_POD(C)\
	TYPESPEC_REGISTER_NAME(C);\
	TYPESPEC_REGISTER_IDENTIFIER(C);\
	TYPESPEC_REGISTER_SIZE(C)


template <class T>
struct TypeSpecName
{
	static const char* value;
}; // TypeSpecName

template <class T>
const char* TypeSpecName<T>::value = nullptr;


template <class T>
struct TypeSpecIdentifier
{
	static size_t value;
}; // TypeSpecIdentifier

template <class T>
size_t TypeSpecIdentifier<T>::value = 0;


template <class T>
struct TypeSpecSize
{
	static unsigned int value;
}; // TypeSpecSize

template <class T>
unsigned int TypeSpecSize<T>::value = 0;

// custom structures

struct TypeSpecInfo
{
	virtual ~TypeSpecInfo() {}

	virtual const char* name() const = 0;

	virtual size_t identifier() const = 0;
};

template <class T>
struct TypeSpecInstanceInfo : public TypeSpecInfo
{
	virtual const char* name() const
	{
		return TypeSpecName<T>::value;
	}

	virtual size_t identifier() const
	{
		return TypeSpecIdentifier<T>::value;
	}
}; // TypeSpecInstanceInfo

template <class T>
TypeSpecInstanceInfo<T>* typespec_make_info()
{
	static TypeSpecInstanceInfo<T> instance;
	return &instance;
}



// utility functions


// convert a type to typespec name
template <class T>
const char* typespec_name_from_value(T)
{
	// If you hit this, a name was not specified for the type.
	assert(0);
	return nullptr;
}

template <class T>
const char* typespec_name_from_value(T*)
{
	return TypeSpecName<T>::value;
}

template <class T>
size_t typespec_identifier_from_value(T)
{
	// If you hit this, an identifier was not specified for the type.
	assert(0);
	return 0;
}

template <class T>
size_t typespec_identifier_from_value(T*)
{
	return TypeSpecIdentifier<T>::value;
}


template <class T>
struct typespec_is_pod
{
	enum
	{
		value = std::is_trivially_constructible<T>::value
		//value = std::is_pod<T>::value
	};
};
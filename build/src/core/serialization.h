// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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

#include <core/logging.h>

#include "typedefs.h"
#include "mem.h"

#include "reflection.h"

// ---------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------


template <bool C>
struct Boolean
{
	static const bool value = C;
	typedef bool value_type;
	constexpr operator bool() const { return value; }
};




//template <bool b>
//using bool_constant = std::integral_constant<bool, b>;


// Taking a note from cereal; I want to support internal and external serializers
enum SerializerType
{
	SerializerTypeUnknown,
	SerializerTypeInternal,
	SerializerTypeExternal
};


// It would also be very flexible if we could support a single 'serialize'
// function or a separate load/save pair. Cereal also supports this.

// ---------------------------------------------------------------------
// If
// ---------------------------------------------------------------------
// We could also use std::conditional; but I like Walter E. Brown's naming
// better, so I've re-created it here.
template <bool, typename, typename>
struct If;

template <typename T, typename F>
struct If<true, T, F>
{
	typedef T value;
};

template <typename T, typename F>
struct If<false, T, F>
{
	typedef F value;
};

template <bool, int, int>
struct IfValue;

template <int T, int F>
struct IfValue<true, T, F>
{
	enum { value = T };
};

template <int T, int F>
struct IfValue<false, T, F>
{
	enum { value = F };
};

template <class T>
struct SerializerTypeSelector
{
	static constexpr SerializerType value = SerializerTypeInternal;
};

#define SERIALIZATION_REGISTER_TYPE_EXTERNAL(T)\
		template <>\
		struct SerializerTypeSelector<T>\
		{\
			static constexpr SerializerType value = SerializerTypeExternal;\
		};\
		template <>\
		struct SerializerTypeSelector<T&>\
		{\
			static constexpr SerializerType value = SerializerTypeExternal;\
		};\



struct SerializerUnknown
{
	template <class Archive, class T>
	static void serialize(Archive& ar, T value)
	{
		assert(0);
		LOGV("serialize unknown\n");
	}
};

struct SerializerClass
{
	template <class Archive, class T>
	static void serialize(Archive& ar, T value)
	{
		LOGV("serialize class\n");
		size_t version = 1;
		value.serialize(ar, version);
//		foo(value);
	}
};

struct SerializerPOD
{
	template <class Archive, class T>
	static void serialize(Archive& ar, T& value)
	{
		LOGV("serialize pod\n");
		ar.save(ar, value);
	}
};

struct SerializeCString
{
	template <class Archive>
	static void serialize(Archive& ar, const char* value)
	{
		LOGV("serialize cstring\n");
	}
};

//struct SerializerProperty
//{
//	template <class Archive, class T>
//	static void serialize(Archive& ar, ClassProperty<T>& property)
//	{
//		LOGV("serialize property '%s', address: %p\n", property.name, property.address);
//		ar & static_cast<T&>(*property.address);
//	}
//};



struct CustomSerializerPOD
{
	template <class Archive, class T>
	static void read(Archive& ar, T& value)
	{
		LOGV("read POD!\n");
	}

	template <class Archive, class T>
	static void write(Archive& ar, T& value)
	{
		LOGV("write POD!\n");
	}
};

struct CustomSerializer
{
	template <class Archive, class T>
	static void read(Archive& ar, T& value)
	{
		LOGV("read!\n");
	}

	template <class Archive, class T>
	static void write(Archive& ar, T& value)
	{
		LOGV("write!\n");
	}
};

template <class Archive, class T, size_t N>
void choose_category_serializer(Archive& ar, T(&value)[N])
{
	LOGV("wtf\n");
	assert(0);
}


template <class Archive, class T>
void choose_category_serializer(Archive& ar, T value)
{
	typedef typename \
		If<reflection::TypeCategory<T>::value == reflection::TypeInfo_POD,
			SerializerPOD,
		typename \
			If<reflection::TypeCategory<T>::value == reflection::TypeInfo_Class,
				SerializerClass,
			SerializerUnknown>
		::value>
	::value serializer;

	serializer::template serialize<Archive, T>(ar, value);
}


struct SerializeRouterPointer
{
	template <class Archive, class T>
	static void choose_serializer(Archive& ar, T value)
	{
		LOGV("choose serialize for a pointer\n");
	}
};


struct SerializeRouterNonPointer
{
	template <class Archive, class T>
	static void choose_serializer(Archive& ar, T value)
	{
		LOGV("choose a serializer for a non-pointer\n");
		choose_category_serializer(ar, value);
	}
};



struct SerializeInternalPointer
{
	template <class Archive, class T>
	static void route(Archive& ar, T&& value)
	{
		LOGV("SerializeInternalPointer\n");
		value.serialize(ar, 1);
	}

	template <class Archive, class T>
	static void route(Archive& ar, T*&& value)
	{
		LOGV("SerializeInternalPointer\n");
		value->serialize(ar, 1);
	}
};

struct SerializeInternalReference
{
	template <class Archive, class T>
	static void route(Archive& ar, T&& value)
	{
		LOGV("SerializeInternalReference\n");
		SerializeInternalPointer::template route<Archive, T>(ar, std::forward<T>(value));
	}
};




struct SerializeExternalPointer
{
	template <class Archive, class T>
	static void route(Archive& ar, T&& value)
	{
		LOGV("SerializeExternalPointer\n");
		serialize(ar, value);
	}

	template <class Archive, class T>
	static void route(Archive& ar, T* value)
	{
		LOGV("SerializeExternalPointer\n");
		serialize(ar, *value);
	}
};

struct SerializeExternalReference
{
	template <class Archive, class T>
	static void route(Archive& ar, T&& value)
	{
		LOGV("SerializeExternalReference\n");
		SerializeExternalPointer::template route<Archive, T>(ar, value);
	}
};


struct SerializerTypeRouterInternal
{
	template <class Archive, class T>
	static void route_type(Archive& ar, T&& value)
	{
		LOGV("SerializerTypeRouterInternal\n");
		typedef typename If<reflection::traits::is_pointer<T>::value,
			SerializeInternalPointer,
		SerializeInternalReference>::value RouterTest;

		RouterTest::template route(ar, std::forward<T>(value));
	}
};

struct SerializerTypeRouterExternal
{
	template <class Archive, class T>
	static void route_type(Archive& ar, T&& value)
	{
		LOGV("SerializerTypeRouterExternal\n");
		typedef typename If<reflection::traits::is_pointer<T>::value,
			SerializeExternalPointer,
		SerializeExternalReference>::value RouterTest;

		RouterTest::template route(ar, value);
	}
};



struct SerializeRoutePOD
{
	template <class Archive, class T>
	static void route(Archive& ar, T&& value)
	{
		LOGV("SerializeRoutePOD\n");

		// POD types never have internal serializers
		ar.save(ar, value);
	}
};

struct SerializeRouteClass
{
	template <class Archive, class T>
	static void route(Archive& ar, T&& value)
	{
		LOGV("SerializeRouteClass\n");

		typedef typename \
			If<SerializerTypeSelector<T>::value == SerializerTypeInternal,
				SerializerTypeRouterInternal,
				SerializerTypeRouterExternal
			>::value type_selector;

		ar.begin_class(ar, value);
		type_selector::route_type(ar, std::forward<T>(value));
		ar.end_class(ar, value);
	}
};

struct SerializeRouteProperty
{
	template <class Archive, class T>
	static void route(Archive& ar, T&& property)
	{
		LOGV("serialize property!\n");

		ar.begin_property(property);
		route_serializer(ar, property.ref);
		ar.end_property(property);


	}
};

template <class Archive, class T>
void route_serializer(Archive& ar, T&& value)
{
	using namespace reflection::traits;

	LOGV("route_serializer\n");

	// Determine if T is a POD-type
	typedef typename\
		If<reflection::TypeCategory<T>::value == reflection::TypeInfo_POD,
			SerializeRoutePOD,
		// else
			typename\
			If<reflection::TypeCategory<T>::value == reflection::TypeInfo_Property,
				SerializeRouteProperty,
			// else
				SerializeRouteClass>::value
		>::value route_selector;

	route_selector::route(ar, std::forward<T>(value));
}

template <class Archive>
class Writer
{
protected:
	Writer() {}

public:
	Boolean<true> is_saving;
	Boolean<false> is_loading;

	// perfect forwarding
	template <class T>
	Archive& operator<<(T&& value)
	{
		route_serializer(*instance(), std::forward<T>(value));
		return *instance();
	}

	Archive* instance()
	{
		return static_cast<Archive*>(this);
	}
};


template <class Archive>
class Reader
{
protected:
	Reader() {}

public:
	Boolean<false> is_saving;
	Boolean<true> is_loading;


	template <class T>
	Archive& operator>>(T& value)
	{
		choose_category_serializer(*instance(), value);
		return *instance();
	}

	template <class T>
	Archive& operator& (T& value)
	{
		return *instance() >> value;
	}

	template <class T>
	Archive& operator>>(const reflection::ClassProperty<T>& property)
	{
		instance()->read_property(property);
		return *instance();
	}

	Archive* instance()
	{
		return static_cast<Archive*>(this);
	}
};


TYPEINFO_REGISTER_TYPE_CATEGORY(int, TypeInfo_POD);
TYPEINFO_REGISTER_TYPE_CATEGORY(float, TypeInfo_POD);
TYPEINFO_REGISTER_TYPE_CATEGORY(char, TypeInfo_POD);
TYPEINFO_REGISTER_TYPE_CATEGORY(bool, TypeInfo_POD);
TYPEINFO_REGISTER_TYPE_CATEGORY(unsigned char, TypeInfo_POD);
TYPEINFO_REGISTER_TYPE_CATEGORY(size_t, TypeInfo_POD);
TYPEINFO_REGISTER_TYPE_CATEGORY(char*, TypeInfo_POD);
TYPEINFO_REGISTER_TYPE_CATEGORY(const char*, TypeInfo_POD);


//namespace serialization
//{
//} // namespace serialization

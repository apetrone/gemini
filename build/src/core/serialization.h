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

#include "typedefs.h"
#include "mem.h"

#include "reflection.h"

// ---------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------

#include <type_traits>


namespace traits
{
	template <class T>
	constexpr bool is_pod(T)
	{
		return std::is_pod<T>::value;
	}


	template <class T>
	constexpr bool is_polymorphic(T)
	{
		return std::is_polymorphic<T>::value;
	}


}


template <bool C>
struct Boolean
{
	static const bool value = C;
	typedef bool value_type;
	constexpr operator bool() const { return value; }
};




// ---------------------------------------------------------------------
// If
// ---------------------------------------------------------------------
template <bool condition, typename T, typename F>
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

struct SerializerUnknown
{
	template <class Archive, class T>
	static void serialize(Archive& ar, T value)
	{
		assert(0);
		fprintf(stdout, "serialize unknown\n");
	}
};

struct SerializerClass
{
	template <class Archive, class T>
	static void serialize(Archive& ar, T& value)
	{
		fprintf(stdout, "serialize class\n");
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
		fprintf(stdout, "serialize pod\n");
		ar.save(ar, value);
	}
};

//struct SerializerProperty
//{
//	template <class Archive, class T>
//	static void serialize(Archive& ar, ClassProperty<T>& property)
//	{
//		fprintf(stdout, "serialize property '%s', address: %p\n", property.name, property.address);
//		ar & static_cast<T&>(*property.address);
//	}
//};



struct CustomSerializerPOD
{
	template <class Archive, class T>
	static void read(Archive& ar, T& value)
	{
		fprintf(stdout, "read POD!\n");
	}

	template <class Archive, class T>
	static void write(Archive& ar, T& value)
	{
		fprintf(stdout, "write POD!\n");
	}
};

struct CustomSerializer
{
	template <class Archive, class T>
	static void read(Archive& ar, T& value)
	{
		fprintf(stdout, "read!\n");
	}

	template <class Archive, class T>
	static void write(Archive& ar, T& value)
	{
		fprintf(stdout, "write!\n");
	}
};

// It would be nice if this logic wasn't tied to a specific read or write operation
// to avoid duplication of the logic.
template <class Archive, class T>
void choose_serializer(Archive& ar, T& value)
{
	typedef typename \
		If<reflection::get_type_category<T>() == reflection::TypeInfo_POD,
			CustomSerializerPOD,
		CustomSerializer>::value serializer;

	serializer::template read<Archive, T>(ar, value);
}



template <class Archive>
class Writer
{
protected:
	Writer() {}

public:
	Boolean<true> is_saving;
	Boolean<false> is_loading;

	template <class T>
	void choose_category_serializer(T value)
	{
		typedef typename \
			If<reflection::get_type_category<T>() == reflection::TypeInfo_POD,
				SerializerPOD,
			typename \
				If<reflection::get_type_category<T>() == reflection::TypeInfo_Class,
					SerializerClass,
				SerializerUnknown>
			::value>
		::value serializer;

		serializer::template serialize<Archive, T>(*instance(), value);
	}

	template <class T>
	Archive& operator<<(T& value)
	{
		instance()->choose_category_serializer(value);
		return *instance();
	}

	template <class T>
	Archive& operator& (T& value)
	{
		return *instance() << value;
	}


	template <class T>
	Archive& operator<<(const reflection::ClassProperty<T>& property)
	{
		instance()->write_property(property);
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
		choose_serializer(*instance(), value);
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


struct TestReader : public Reader<TestReader>
{
	template <class T>
	void read_property(const reflection::ClassProperty<T>& property)
	{
		fprintf(stdout, "READ property '%s', address: %p\n", property.name, &property.ref);
		(*instance()) & property.ref;
	}
};




struct TestArchive : public Writer<TestArchive>
{
	int val;
	float fl_value;

	TestArchive()
	{
		val = -1;
	}


	template <class Archive, class T>
	void save(Archive& ar, T& value)
	{
		fprintf(stdout, "should save something\n");
		value.serialize(ar, 1);
	}

	template <class Archive, class T>
	void save(Archive& ar, const T& value)
	{
		// Couldn't deduce the type.
		assert(0);
	}

	template <class Archive>
	void save(Archive& ar, int& value)
	{
		val = value;
	}

	template <class Archive>
	void save(Archive& ar, float& value)
	{
		fl_value = value;
	}

	template <class Archive>
	void save(Archive& ar, bool& value)
	{
		val = value;
	}

	template <class Archive>
	void save(Archive& ar, char& value)
	{

	}

	template <class T>
	void write_property(const reflection::ClassProperty<T>& property)
	{
		fprintf(stdout, "serialize property '%s', address: %p\n", property.name, property.address);
		T& value = static_cast<T&>(*property.address);
		(*instance()) & value;
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

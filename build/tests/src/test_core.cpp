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
#include "unit_test.h"

#include <core/argumentparser.h>
#include <core/array.h>
#include <core/color.h>
#include <core/datastream.h>
#include <core/fixedarray.h>
#include <core/fixedsizequeue.h>
#include <core/hashset.h>
#include <core/mathlib.h>

#include <core/mem.h>

#include <core/str.h>
#include <core/stackstring.h>
#include <core/threadsafequeue.h>
#include <core/util.h>


#include <platform/platform.h>

#include <vector>
#include <string>
#include <assert.h>
#include <stdio.h>

using namespace core;
using namespace core::util;

// ---------------------------------------------------------------------
// ArgumentParser
// ---------------------------------------------------------------------
void test_argumentparser()
{
	TEST_CATEGORY(ArgumentParser);

	const char* docstring = R"(
Usage:
	--test=<path>

Options:
	-h, --help  Show this help screen
	--version  Display the version number
	)";

	std::vector<std::string> arguments;

	arguments.push_back("--test=target_path");

	argparse::ArgumentParser parser;
	argparse::VariableMap vm;
	parser.parse(docstring, arguments, vm, "1.0.0-alpha");

	std::string path = vm["--test"];

	TEST_VERIFY(path == "target_path", single_argument);

}


// ---------------------------------------------------------------------
// Array
// ---------------------------------------------------------------------
void test_array()
{
	TEST_CATEGORY(Array);

	Array<int> a;

	TEST_VERIFY(a.empty(), is_empty);
	TEST_VERIFY(a.size() == 0, size_empty);

	a.push_back(0);
	TEST_VERIFY(a.size() == 1, size_after_adding_one);


	for (size_t i = 0; i < 14; ++i)
	{
		a.push_back(i+1);
	}

	TEST_VERIFY(a.size() == 15, size_after_adding_14);

	TEST_VERIFY(a[10] == 10, array_index);

	// push a 16th element (should be OK)
	a.push_back(16);

	// push a 17th element (should force a growth)
	a.push_back(17);
	TEST_VERIFY(a.size() == 17, size_after_growth);

	// verify the n-th element
	TEST_VERIFY(a[10] == 10, array_index_after_growth);
	
	
	
	Array<int> abc(6);
	abc.push_back(30);
	abc.push_back(60);
	abc.push_back(90);
	
	// test const value iteration
	for (const int& value : abc)
	{
	}

	abc.erase(60);

	TEST_VERIFY(abc.size() == 2, size_after_erase);
	for (const int& value : abc)
	{
		fprintf(stdout, "value: %i\n", value);
	}
	

	// test iterators
	Array<int>::iterator iter = abc.begin();
	iter++;
	++iter;

	Array<int>::reverse_iterator riter = abc.rbegin();
	riter++;
	++riter;

	// now try with a custom type to make sure the ctor/dtor
	// are called.
	static size_t _local_counter = 0;
	struct CustomType
	{
		CustomType()
		{
			_local_counter++;
		}

		~CustomType()
		{
			--_local_counter;
		}
	};

	Array<CustomType> b(4);
	TEST_VERIFY(_local_counter == 4, constructor_called_n_times);
	b.clear();

	TEST_VERIFY(_local_counter == 0, destructor_called_n_times);
}

// ---------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------
void test_color()
{
	TEST_CATEGORY(Color);

	Color red(255, 0, 0, 255);

	Color temp;
	float red_float[4] = {1.0f, 0.0f, 0.0f, 1.0f};
	temp = Color::from_float_pointer(red_float, 4);
	TEST_VERIFY(temp == red, from_float_pointer);

	uint32_t u32_color = red.as_uint32();
	Color int_color = Color::from_int(u32_color);
	TEST_VERIFY(int_color == red, from_int);

	Color ubyte_test(0, 128, 255, 32);
	unsigned char ubyte[] = {0, 128, 255, 32};
	Color ubyte_color = Color::from_ubyte(ubyte);

	TEST_VERIFY(ubyte_color == ubyte_test, from_ubyte);
}


// ---------------------------------------------------------------------
// DataStream
// ---------------------------------------------------------------------
void test_datastream()
{
	{
		TEST_CATEGORY(MemoryStream);

		const size_t DATA_SIZE = 256;
		unsigned char data[ DATA_SIZE] = {0};

		MemoryStream ms;
		ms.init((char*)data, DATA_SIZE );

		int test_values[2] = {32, 64};
		float test_float = 3.2f;
		void* test_pointer = &data[0];
		uint64_t test_uint64 = UINT64_MAX;

		ms.write(test_values[0]);
		ms.write(test_values[1]);
		ms.write(test_float);
		ms.write(test_pointer);
		ms.write(test_uint64);

		ms.rewind();

		int out_values[2];
		float out_float;
		void* out_pointer = 0;
		uint64_t out_uint64;

		ms.read(out_values[0]);
		ms.read(out_values[1]);
		ms.read(out_float);
		ms.read(out_pointer);
		ms.read(out_uint64);

		TEST_VERIFY((
			out_values[0] == test_values[0] &&
			out_values[1] == test_values[1] &&
			out_float == test_float &&
			out_pointer == test_pointer &&
			out_uint64 == test_uint64
			),
			pod_read_write
		);

		TEST_VERIFY(ms.get_data_size() == DATA_SIZE, get_data_size);
		TEST_VERIFY(ms.get_data() == data, get_data);
	}
}


// ---------------------------------------------------------------------
// FixedArray
// ---------------------------------------------------------------------
void test_fixedarray()
{
	TEST_CATEGORY(FixedArray);

	FixedArray<int> int_array;
	int_array.allocate(32);
	TEST_VERIFY(int_array.size() == 32, size);

	int_array[15] = 0xbeef;
	TEST_VERIFY(int_array[15] == 0xbeef, array_index);

	TEST_VERIFY(int_array.empty() == false, empty_when_not_empty);

	int_array.clear();
	TEST_VERIFY(int_array.size() == 0, clear);

	int_array.empty();
	TEST_VERIFY(int_array.empty() == true, empty_when_empty);


	// test with a smaller size
	int_array.allocate(2);
	TEST_VERIFY(int_array.size() == 2, reallocate);

	int_array[0] = 128;
	int_array[1] = 256;


	int values[2];
	size_t index = 0;
	for (auto& i : int_array)
	{
		values[index++] = i;
	}

	TEST_VERIFY(values[0] == 128 && values[1] == 256, ranged_for_loop);
}


// ---------------------------------------------------------------------
// FixedSizeQueue
// ---------------------------------------------------------------------
void test_fixedsizequeue()
{
	TEST_CATEGORY(FixedSizeQueue);

	FixedSizeQueue<int, 4> small_queue;

	small_queue.push_back(32);
	small_queue.push_back(64);
	small_queue.push_back(128);
	small_queue.push_back(256);

	TEST_VERIFY(small_queue.size() == 4, size);

	TEST_VERIFY(small_queue.empty() == false, empty_when_not_empty);

	TEST_VERIFY(small_queue.push_back(123) == false, push_fails_when_full);

	int value = small_queue.pop();
	TEST_VERIFY(value == 256, lifo_pop);

	int values[3];
	values[0] = small_queue.pop();
	values[1] = small_queue.pop();
	values[2] = small_queue.pop();
	TEST_VERIFY(values[0] == 128 && values[1] == 64 && values[2] == 32, pop);

	TEST_VERIFY(small_queue.empty() == true, empty_when_empty);
}


// ---------------------------------------------------------------------
// HashSet
// ---------------------------------------------------------------------
void test_hashset()
{
	TEST_CATEGORY(HashSet);

	HashSet<std::string, int> dict;
	dict["first"] = 1;
	TEST_VERIFY(dict.size() == 1, operator_insert);
	dict["second"] = 2;
	dict["third"] = 3;
	dict["fourth"] = 4;

	TEST_VERIFY(dict.size() == 4, size);

	TEST_VERIFY(dict.has_key("third"), has_key);

	dict.insert(std::pair<std::string, int>("fifth", 5));
	TEST_VERIFY(dict.size() == 5, insert);

	dict.clear();
	TEST_VERIFY(dict.size() == 0, clear);


	HashSet<std::string, void*> custom_capacity(4096);
	TEST_VERIFY(custom_capacity.capacity() == 4096, capacity);


	int second = dict.get("second");
	TEST_VERIFY(second == 2, get);
}


// ---------------------------------------------------------------------
// CircularBuffer
// ---------------------------------------------------------------------
void test_circularbuffer()
{
	TEST_CATEGORY(CircularBuffer);

	CircularBuffer<int, 3> cb;
	int& a = cb.next();
	a = 30;

	int& b = cb.next();
	b = 60;

	int& c = cb.next();
	c = 90;

	int d = cb.next();
	TEST_VERIFY(d == a, circular_buffer);
}

// ---------------------------------------------------------------------
// interpolation
// ---------------------------------------------------------------------


// ---------------------------------------------------------------------
// mathlib
// ---------------------------------------------------------------------
void test_mathlib()
{
	TEST_CATEGORY(mathlib);

	const float forty_five_degrees = 45.0f;
	const float forty_five_degrees_in_radians = 0.785398185f;


	float temp = mathlib::degrees_to_radians(45);
	TEST_VERIFY(temp == forty_five_degrees_in_radians, degrees_to_radians);

	temp = mathlib::radians_to_degrees(temp);
	TEST_VERIFY(temp == 45, radians_to_degrees);
}

// ---------------------------------------------------------------------
// memory
// ---------------------------------------------------------------------
void test_memory()
{
	TEST_CATEGORY(memory);

	TEST_VERIFY(1, sanity);
}

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




template <class T>
class Archive
{
public:

	template <class V>
	Archive& operator<<(V& value)
	{
		loading = false;
		instance()->Save(*instance(), value);
		return *this;
	}

	template <class V>
	Archive& operator>>(V& value)
	{
		loading = true;
		instance()->Load(*instance(), value);
		return *this;
	}

	bool is_loading() const { return loading; }

private:
	T* instance()
	{
		return static_cast<T*>(this);
	}

private:
	bool loading;
};

#define REFLECTION_BEGIN(class_name) static ClassProperty* reflection_fields(size_t& total_properties)\
{\
typedef class_name reflection_class_type;\
static ClassProperty properties[] = {

#define REFLECTION_END() };\
static size_t property_count = sizeof(properties)/sizeof(ClassProperty);\
total_properties = property_count;\
return properties;\
}

#define REFLECTION_PROPERTY(property) ClassProperty(#property, offsetof(reflection_class_type, property))

struct ClassProperty
{
	const char* name;
	size_t offset;

	ClassProperty(
		const char* property_name,
		size_t property_offset) :
		name(property_name),
		offset(property_offset)
	{
	}
};

struct TestArchive : public Archive<TestArchive>
{
	int val;
	float fl_value;

	TestArchive()
	{
		val = -1;
	}

	template <class Archive, class T>
	void Save(Archive& ar, T& value)
	{
		fprintf(stdout, "should save something\n");
		value.serialize(ar, 1);
	}

	template <class Archive, class T>
	void Load(Archive& ar, T& value)
	{
		fprintf(stdout, "should load something\n");
		value.serialize(ar, 1);
	}


	// ---------------------------------------------------------------------
	// write
	// ---------------------------------------------------------------------

	template <class Archive, class T>
	TestArchive& write(Archive& ar, const ClassProperty& prop, T& value)
	{
		// nothing defined to handle this type
		assert(0);
		return *this;
	}


	template <class Archive>
	TestArchive& write(Archive& ar, const ClassProperty& prop, int& value)
	{
		val = value;
		return *this;
	}

	template <class Archive>
	TestArchive& write(Archive& ar, const ClassProperty& prop, float& value)
	{
		fl_value = value;
		return *this;
	}

	// ---------------------------------------------------------------------
	// read
	// ---------------------------------------------------------------------

	template <class Archive, class T>
	TestArchive& read(Archive& ar, const ClassProperty& prop, T& value)
	{
		// nothing defined to handle this type
		assert(0);
		return *this;
	}

	template <class Archive>
	TestArchive& read(Archive& ar, const ClassProperty& prop, int& value)
	{
		value = val;
		return *this;
	}
	
	template <class Archive>
	TestArchive& read(Archive& ar, const ClassProperty& prop, float& value)
	{
		value = fl_value;
		return *this;
	}


	template <class T>
	TestArchive& route(const ClassProperty& property, T& value)
	{
		fprintf(stdout, "name: %s\n", property.name);
		fprintf(stdout, "offset: %zu\n", property.offset);
		fprintf(stdout, "ptr: %p\n", &value);

		if (is_loading())
		{
			read(*this, property, value);
		}
		else
		{
			write(*this, property, value);
		}

		return *this;
	}
};


struct Test
{
	int value;
	float precision;

	template <class Archive>
	void serialize(Archive& ar, const size_t version)
	{
		fprintf(stdout, "serialize for test: %zu\n", offsetof(Test, value));

		ar.route(ClassProperty("value", offsetof(Test, value)), value);
		ar.route(ClassProperty("precision", offsetof(Test, precision)), precision);
	}

//	REFLECTION_BEGIN(Test)
//		REFLECTION_PROPERTY(value)
//	REFLECTION_END()
};

struct Test2
{
	int value;

	template <class Archive>
	void serialize(Archive& ar, const size_t version)
	{
		fprintf(stdout, "serialize for test2 \n");
	}
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



void test_serialization()
{
	TestArchive a;

	Test test;
	test.value = 32;
	test.precision = 8.231f;

	a << test;


	Test temp;
	a >> temp;

//	assert(test.value == temp.value);
	int z = 723;

}

// ---------------------------------------------------------------------
// StackString
// ---------------------------------------------------------------------
void test_stackstring()
{
	TEST_CATEGORY(StackString);

	StackString<128> s1;

	s1 = "name";
	TEST_VERIFY(s1.size() == 4, size);

	s1.clear();
	TEST_VERIFY(s1.size() == 0, clear);

	TEST_VERIFY(s1.is_empty(), is_empty);

	s1 = "/usr/bin/git";
	char* last_slash = s1.find_last_slash();
	TEST_VERIFY(*last_slash == '/' && last_slash == &s1[8], find_last_slash);

	StackString<128> base = s1.basename();
	TEST_VERIFY(base == "git", basename);

	StackString<128> directory = s1.dirname();
	TEST_VERIFY(directory == "/usr/bin", dirname);

	StackString<128> filename = "c:\\Users\\Administrator\\archive.zip";
	StackString<128> ext = filename.extension();
	TEST_VERIFY(ext == "zip", extension);

	filename = "c:/abnormal/path\\mixed\\slashes/with\\archive.zip";
	filename.normalize('\\');

	TEST_VERIFY(filename == "c:\\abnormal\\path\\mixed\\slashes\\with\\archive.zip", normalize);

	filename = "test";
	filename.append("_one_two");
	TEST_VERIFY(filename == "test_one_two", append);

	s1 = "whitespace sucks    ";
	s1 = s1.strip_trailing(' ');
	TEST_VERIFY(s1 == "whitespace sucks", strip_trailing);

	s1 = "orion gemini constellation";
	TEST_VERIFY(s1.startswith("orion"), startswith);
}


// ---------------------------------------------------------------------
// str
// ---------------------------------------------------------------------
void test_str()
{
	TEST_CATEGORY(str);

	int result = 0;

	const char static_buffer[] = "items: 30, value: 2.40";
	char* output = str::format("items: %i, value: %2.2f", 30, 2.4f);

	result = core::str::case_insensitive_compare(static_buffer, output, 0);
	TEST_VERIFY(result == 0, format);

	// llu is not recognized under 32-bit environments
	char local[128] = {0};
	result = core::str::sprintf(local, 128, "test_of_sprintf: %i\n", 4096);
	fprintf(stdout, "result: %s\n", local);
	fprintf(stdout, "result is: %i\n", result);
	TEST_VERIFY(result == 16, sprintf);

	memset(local, 0, 128);
	core::str::copy(local, "PAGE_SIZE", 0);
	TEST_VERIFY(core::str::case_insensitive_compare(local, "PAGE_SIZE", 0) == 0, copy);

	TEST_VERIFY(core::str::len(local) == 9, len);

	core::str::cat(local, " = 4096");
	TEST_VERIFY(core::str::case_insensitive_compare(local, "PAGE_SIZE = 4096", 0) == 0, cat);

	char base[] = "string test number one";
	result = core::str::case_insensitive_compare(base, "string test number one", 0);
	TEST_VERIFY(result == 0, case_insensitive_compare);
}


// ---------------------------------------------------------------------
// util
// ---------------------------------------------------------------------
void test_util()
{
	TEST_CATEGORY(util);

	float value = util::random_range(0.0f, 1.0f);
	TEST_VERIFY(0.0f <= value && value <= 1.0f, random_range0);

	value = util::random_range(0.0f, 1.0f);
	TEST_VERIFY(0.0f <= value && value <= 1.0f, random_range1);

	value = util::random_range(0.0f, 1.0f);
	TEST_VERIFY(0.0f <= value && value <= 1.0f, random_range2);

	value = util::random_range(0.0f, 1.0f);
	TEST_VERIFY(0.0f <= value && value <= 1.0f, random_range3);

	value = util::random_range(0.0f, 1.0f);
	TEST_VERIFY(0.0f <= value && value <= 1.0f, random_range4);
}

int main(int, char**)
{
	core::memory::startup();

	test_argumentparser();
	test_array();
	test_color();
	test_datastream();
	test_fixedarray();
	test_fixedsizequeue();
	test_hashset();
	test_circularbuffer();
	test_mathlib();
	test_memory();
	test_stackstring();
	test_str();
	test_util();

	core::memory::shutdown();

	return 0;
}

#if 0
#define TEST(name) void test_##name(UnitTestCategory& category)

TEST(StackString)
{

}
#endif

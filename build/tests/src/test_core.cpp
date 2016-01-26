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
#include <core/core.h>
#include <core/mem.h>

#include <core/str.h>
#include <core/stackstring.h>
#include <core/threadsafequeue.h>
#include <core/util.h>


#include <platform/platform.h>

#include <vector>
#include <string>
#include <assert.h>


using namespace core;
using namespace core::util;




// ---------------------------------------------------------------------
// ArgumentParser
// ---------------------------------------------------------------------
UNITTEST(ArgumentParser)
{
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

	TEST_ASSERT(path == "target_path", single_argument);

}

// ---------------------------------------------------------------------
// Array
// ---------------------------------------------------------------------
UNITTEST(Array)
{
	Array<int> a;

	TEST_ASSERT(a.empty(), is_empty);
	TEST_ASSERT(a.size() == 0, size_empty);

	a.push_back(0);
	TEST_ASSERT(a.size() == 1, size_after_adding_one);


	for (size_t i = 0; i < 14; ++i)
	{
		a.push_back(i+1);
	}

	TEST_ASSERT(a.size() == 15, size_after_adding_14);

	TEST_ASSERT(a[10] == 10, array_index);

	// push a 16th element (should be OK)
	a.push_back(16);

	// push a 17th element (should force a growth)
	a.push_back(17);
	TEST_ASSERT(a.size() == 17, size_after_growth);

	// verify the n-th element
	TEST_ASSERT(a[10] == 10, array_index_after_growth);



	Array<int> abc(6);
	abc.push_back(30);
	abc.push_back(60);
	abc.push_back(90);

	// test const value iteration
	for (const int& value : abc)
	{
	}

	abc.erase(60);

	abc.pop_back();

	TEST_ASSERT(abc.size() == 1, size_after_erase);
	for (const int& value : abc)
	{
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
	TEST_ASSERT(_local_counter == 4, constructor_called_n_times);
	b.clear();

	TEST_ASSERT(_local_counter == 0, destructor_called_n_times);

	// test resize with default value
	Array<int> rd;
	rd.push_back(30);
	rd.push_back(60);
	rd.push_back(90);
	rd.resize(4, 0);

	TEST_ASSERT(rd[0] == 30, resize_default_keep_existing);
	TEST_ASSERT(rd[3] == 0, resize_default_set);


	// test qsort
	Array<int> values;
	values.push_back(7);
	values.push_back(2);
	values.push_back(1);
	values.push_back(6);
	values.push_back(8);
	values.push_back(5);
	values.push_back(3);
	values.push_back(4);
	core::sort<core::quicksort>(values.begin(), values.end());

	int sorted_list[] = {1, 2, 3, 4, 5, 6, 7, 8};
	for (size_t index = 0; index < values.size(); ++index)
	{
		TEST_ASSERT(values[index] == sorted_list[index], quicksort);
	}
}

// ---------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------
UNITTEST(Color)
{
	Color red(1.0f, 0, 0, 1.0f);

	Color temp;
	float red_float[4] = {1.0f, 0.0f, 0.0f, 1.0f};
	temp = Color::from_float_pointer(red_float, 4);
	TEST_ASSERT(temp == red, from_float_pointer);

	uint32_t u32_color = red.as_uint32();
	Color int_color = Color::from_int(u32_color);
	TEST_ASSERT(int_color == red, from_int);

	Color ubyte_test = Color::from_rgba(0, 128, 255, 32);
	unsigned char ubyte[] = {0, 128, 255, 32};
	Color ubyte_color = Color::from_ubyte(ubyte);

	TEST_ASSERT(ubyte_color == ubyte_test, from_ubyte);
}


// ---------------------------------------------------------------------
// DataStream
// ---------------------------------------------------------------------
UNITTEST(DataStream)
{
	{
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

		TEST_ASSERT((
			out_values[0] == test_values[0] &&
			out_values[1] == test_values[1] &&
			out_float == test_float &&
			out_pointer == test_pointer &&
			out_uint64 == test_uint64
			),
			pod_read_write
		);

		TEST_ASSERT(ms.get_data_size() == DATA_SIZE, get_data_size);
		TEST_ASSERT(ms.get_data() == data, get_data);
	}
}


// ---------------------------------------------------------------------
// FixedArray
// ---------------------------------------------------------------------
UNITTEST(FixedArray)
{
	FixedArray<int> int_array;
	int_array.allocate(32);
	TEST_ASSERT(int_array.size() == 32, size);

	int_array[15] = 0xbeef;
	TEST_ASSERT(int_array[15] == 0xbeef, array_index);

	TEST_ASSERT(int_array.empty() == false, empty_when_not_empty);

	int_array.clear();
	TEST_ASSERT(int_array.size() == 0, clear);

	int_array.empty();
	TEST_ASSERT(int_array.empty() == true, empty_when_empty);


	// test with a smaller size
	int_array.allocate(2);
	TEST_ASSERT(int_array.size() == 2, reallocate);

	int_array[0] = 128;
	int_array[1] = 256;


	int values[2];
	size_t index = 0;
	for (auto& i : int_array)
	{
		values[index++] = i;
	}

	TEST_ASSERT(values[0] == 128 && values[1] == 256, ranged_for_loop);
}


// ---------------------------------------------------------------------
// FixedSizeQueue
// ---------------------------------------------------------------------
UNITTEST(FixedSizeQueue)
{
	FixedSizeQueue<int, 4> small_queue;

	small_queue.push_back(32);
	small_queue.push_back(64);
	small_queue.push_back(128);
	small_queue.push_back(256);

	TEST_ASSERT(small_queue.size() == 4, size);

	TEST_ASSERT(small_queue.empty() == false, empty_when_not_empty);

	TEST_ASSERT(small_queue.push_back(123) == false, push_fails_when_full);

	int value = small_queue.pop();
	TEST_ASSERT(value == 256, lifo_pop);

	int values[3];
	values[0] = small_queue.pop();
	values[1] = small_queue.pop();
	values[2] = small_queue.pop();
	TEST_ASSERT(values[0] == 128 && values[1] == 64 && values[2] == 32, pop);

	TEST_ASSERT(small_queue.empty() == true, empty_when_empty);
}


// ---------------------------------------------------------------------
// HashSet
// ---------------------------------------------------------------------
UNITTEST(HashSet)
{
	HashSet<std::string, int> dict;
	dict["first"] = 1;
	TEST_ASSERT(dict.size() == 1, operator_insert);
	dict["second"] = 2;
	dict["third"] = 3;
	dict["fourth"] = 4;

	TEST_ASSERT(dict.size() == 4, size);

	TEST_ASSERT(dict.has_key("third"), has_key);

	dict.insert(std::pair<std::string, int>("fifth", 5));
	TEST_ASSERT(dict.size() == 5, insert);

	dict.clear();
	TEST_ASSERT(dict.size() == 0, clear);


	HashSet<std::string, void*> custom_capacity(4096);
	TEST_ASSERT(custom_capacity.capacity() == 4096, capacity);

	int second = dict.get("second");
	TEST_ASSERT(second == 2, get);


	bool has_two = false;
	HashSet<int, int> repop_test(5);
	repop_test[0] = 0;
	repop_test[1] = 1;
	repop_test[2] = 2;
	has_two = repop_test.has_key(2);
	repop_test[3] = 3;
	has_two = repop_test.has_key(2);
	repop_test[4] = 4;
	has_two = repop_test.has_key(2);


	// test using another allocator
	HashSet<int, int, core::util::hash<int>, core::memory::GlobalAllocatorType> test(32, 2, core::memory::global_allocator());

	test.insert(HashSet<int, int>::value_type(30, 72));

	int z = test[30];
	TEST_ASSERT(z == 72, hash_with_global_allocator);
}


// ---------------------------------------------------------------------
// CircularBuffer
// ---------------------------------------------------------------------
UNITTEST(CircularBuffer)
{
	CircularBuffer<int, 3> cb;
	int& a = cb.next();
	a = 30;

	int& b = cb.next();
	b = 60;

	int& c = cb.next();
	c = 90;

	int d = cb.next();
	TEST_ASSERT(d == a, circular_buffer);
}

// ---------------------------------------------------------------------
// interpolation
// ---------------------------------------------------------------------


// ---------------------------------------------------------------------
// mathlib
// ---------------------------------------------------------------------
UNITTEST(mathlib)
{
	const float forty_five_degrees = 45.0f;
	const float forty_five_degrees_in_radians = 0.785398185f;


	float temp = mathlib::degrees_to_radians(45);
	TEST_ASSERT(temp == forty_five_degrees_in_radians, degrees_to_radians);

	temp = mathlib::radians_to_degrees(temp);
	TEST_ASSERT(temp == 45, radians_to_degrees);
}

// ---------------------------------------------------------------------
// memory
// ---------------------------------------------------------------------
UNITTEST(memory)
{
	TEST_ASSERT(1, sanity);
}


#include <core/serialization.h>

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

#include <core/reflection.h>
using namespace reflection;






template <typename T>
class TD;

template <typename T>
void foo(T& param)
{
	TD<T> t_type;
	TD<decltype(param)> param_type;
}

struct Test
{
	TYPEINFO_DECLARE_CLASS(Test);

	int value;
	float precision;

	template <class Archive>
	void serialize(Archive& ar, const size_t version)
	{
//		LOGV("serialize for test: %zu\n", offsetof(Test, value));
		ar & TYPEINFO_PROPERTY(value);
//		ar & TYPEINFO_PROPERTY(precision);
	}
};

TYPEINFO_REGISTER_TYPE_NAME(Test, Test);
TYPEINFO_REGISTER_TYPE_INFO(Test);
TYPEINFO_REGISTER_TYPE_CATEGORY(Test, TypeInfo_Class);

struct Test2 : public Test
{
	TYPEINFO_DECLARE_CLASS(Test2);

	int test2_value;

	template <class Archive>
	void serialize(Archive& ar, const size_t version)
	{
		LOGV("serialize for test2 \n");

//		ar & TYPEINFO_PROPERTY(test2_value);
		ar & test2_value;
	}
};

TYPEINFO_REGISTER_TYPE_NAME(Test2, Test2);
TYPEINFO_REGISTER_TYPE_INFO(Test2);
TYPEINFO_REGISTER_TYPE_BASE(Test2, Test);
TYPEINFO_REGISTER_TYPE_CATEGORY(Test2, TypeInfo_Class);

struct Test3 : public Test2
{
	TYPEINFO_DECLARE_CLASS(Test3);

	int test3_value;

	template <class Archive>
	void serialize(Archive& ar, const size_t version)
	{
		LOGV("serialize for test3\n");
//		ar & TYPEINFO_PROPERTY(test3_value);
		ar & test3_value;
	}
}; // Test3

TYPEINFO_REGISTER_TYPE_NAME(Test3, Test3);
TYPEINFO_REGISTER_TYPE_INFO(Test3);
TYPEINFO_REGISTER_TYPE_BASE(Test3, Test2);


UNITTEST(Serialization)
{
	// The impetus behind writing serialization has to do with the UI code
	// I would like to define a layout in JSON and then read that back in
	// to create the interface panels.

	// There's a bit of a learning curve, but it's healthy.
	// I will start simple to begin with and serialize POD types first.
	// Then, I'll move on and create data structures and try to clean up
	// as I go.

	Test test;
	test.value = 32;
	test.precision = 8.231f;

//	writer << test;

//	int value = 30;
//	writer << value;
//
//	float fl_value = 3.0f;
//	writer << fl_value;
//
//	char c = 'a';
//	writer << c;

//	writer << test;


//	int blah = 72;
//	reader >> make_class_property("blah", blah);

//	int value;
//	writer << ClassProperty<int>("test", &value);

//	Test temp;
//	a >> temp;

#if 0
	// get type info using a static type
	const reflection::TypeInfo* typeinfo = reflection::get_type_info<Test>();
	LOGV("type is: '%s'\n", typeinfo->type_identifier());
	assert(typeinfo->is_same_type(typeinfo));
	assert(typeinfo->is_same_type("Test"));

	// get the type info using the qualified class name
	const reflection::TypeInfo* ti = reflection::TypeRegistry::const_instance().get_type_info("Test");
	assert(ti == typeinfo);

	// get the type info using the instance
	const reflection::TypeInfo* typeinfos = test.get_type_info();
	assert(typeinfos == ti);


	// try a derived class
	const reflection::TypeInfo* t2i = reflection::get_type_info<Test2>();
	LOGV("t2i is '%s'\n", t2i->type_identifier());

	const reflection::TypeInfo* baseclass = t2i->get_base_type();
	assert(baseclass == typeinfo);

	// try test3
	Test3 test3;
	const reflection::TypeInfo* t3i = test3.get_type_info();
	assert(t3i);
	assert(t3i->get_base_type() == t2i);

	// try to walk the hierarchy
	const reflection::TypeInfo* curr = t3i;
	while(curr)
	{
		LOGV("-> %s\n", curr->type_identifier());
		curr = curr->get_base_type();
	}
#endif




//	int* p = TypeConstructor<int>::Construct();
//	*p = 72;

//	TypeDestructor<int>::Destruct(p);
}



// ---------------------------------------------------------------------
// StackString
// ---------------------------------------------------------------------
UNITTEST(stackstring)
{
	StackString<128> s1;

	s1 = "name";
	TEST_ASSERT(s1.size() == 4, size);

	s1.clear();
	TEST_ASSERT(s1.size() == 0, clear);

	TEST_ASSERT(s1.is_empty(), is_empty);

	s1 = "/usr/bin/git";
	char* last_slash = s1.find_last_slash();
	TEST_ASSERT(*last_slash == '/' && last_slash == &s1[8], find_last_slash);

	StackString<128> base = s1.basename();
	TEST_ASSERT(base == "git", basename);

	StackString<128> directory = s1.dirname();
	TEST_ASSERT(directory == "/usr/bin", dirname);

	StackString<128> filename = "c:\\Users\\Administrator\\archive.zip";
	StackString<128> ext = filename.extension();
	TEST_ASSERT(ext == "zip", extension);

	filename = "c:/abnormal/path\\mixed\\slashes/with\\archive.zip";
	filename.normalize('\\');

	TEST_ASSERT(filename == "c:\\abnormal\\path\\mixed\\slashes\\with\\archive.zip", normalize);

	filename = "test";
	filename.append("_one_two");
	TEST_ASSERT(filename == "test_one_two", append);

	s1 = "whitespace sucks    ";
	s1 = s1.strip_trailing(' ');
	TEST_ASSERT(s1 == "whitespace sucks", strip_trailing);

	s1 = "orion gemini constellation";
	TEST_ASSERT(s1.startswith("orion"), startswith);
}


// ---------------------------------------------------------------------
// str
// ---------------------------------------------------------------------
UNITTEST(str)
{
	int result = 0;

	const char static_buffer[] = "items: 30, value: 2.40";
	char* output = str::format("items: %i, value: %2.2f", 30, 2.4f);

	result = core::str::case_insensitive_compare(static_buffer, output, 0);
	TEST_ASSERT(result == 0, format);

	// llu is not recognized under 32-bit environments
	char local[128] = {0};
	result = core::str::sprintf(local, 128, "test_of_sprintf: %i\n", 4096);

	// should be a total of 22 characters:
	// 17 up to the first argument
	// 4 for the integral
	// 1 for \n
	TEST_ASSERT(result == 22, sprintf);

	memset(local, 0, 128);
	core::str::copy(local, "PAGE_SIZE", 0);
	TEST_ASSERT(core::str::case_insensitive_compare(local, "PAGE_SIZE", 0) == 0, copy);

	TEST_ASSERT(core::str::len(local) == 9, len);

	core::str::cat(local, " = 4096");
	TEST_ASSERT(core::str::case_insensitive_compare(local, "PAGE_SIZE = 4096", 0) == 0, cat);

	char base[] = "string test number one";
	result = core::str::case_insensitive_compare(base, "string test number one", 0);
	TEST_ASSERT(result == 0, case_insensitive_compare);
}


// ---------------------------------------------------------------------
// util
// ---------------------------------------------------------------------
UNITTEST(util)
{
	float value = util::random_range(0.0f, 1.0f);
	TEST_ASSERT(0.0f <= value && value <= 1.0f, random_range0);

	value = util::random_range(0.0f, 1.0f);
	TEST_ASSERT(0.0f <= value && value <= 1.0f, random_range1);

	value = util::random_range(0.0f, 1.0f);
	TEST_ASSERT(0.0f <= value && value <= 1.0f, random_range2);

	value = util::random_range(0.0f, 1.0f);
	TEST_ASSERT(0.0f <= value && value <= 1.0f, random_range3);

	value = util::random_range(0.0f, 1.0f);
	TEST_ASSERT(0.0f <= value && value <= 1.0f, random_range4);
}

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <stack>


class CustomJSONAllocator : public rapidjson::CrtAllocator
{
public:
	static const bool kNeedFree = true;
	void* Malloc(size_t size) {
		if (size) //  behavior of malloc(0) is implementation defined.
			return std::malloc(size);
		else
			return NULL; // standardize to returning NULL.
	}
	void* Realloc(void* originalPtr, size_t originalSize, size_t newSize) {
		(void)originalSize;
		if (newSize == 0) {
			std::free(originalPtr);
			return NULL;
		}
		return std::realloc(originalPtr, newSize);
	}
	static void Free(void *ptr) { std::free(ptr); }
};


class JsonWriter : public Writer<JsonWriter>
{
private:
	rapidjson::Document doc;
	rapidjson::StringBuffer buffer;

	std::stack<rapidjson::Value*> object_stack;
public:

	JsonWriter()
	{
		doc.SetObject();
		begin_object();
	}

	~JsonWriter()
	{
	}

	void generate_document()
	{
		rapidjson::Value* object = object_stack.top();
		size_t stacksize = object_stack.size();
		LOGV("stack size final: %zu\n", stacksize);
		assert(object_stack.size() == 1);
		doc.AddMember("object", *object, doc.GetAllocator());
		end_object();


		buffer.Clear();
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
	}

	const char* get_string()
	{
		return buffer.GetString();
	}


	template <class T>
	void save_property(const ClassProperty<T>& property)
	{
		rapidjson::Value* value = object_stack.top();
		assert(value->IsObject());

		rapidjson::Value prop = get_value(*instance(), property);
		value->AddMember(rapidjson::StringRef(property.name), prop, doc.GetAllocator());
	}

	rapidjson::Value* begin_object()
	{
		rapidjson::Value* value = new rapidjson::Value(rapidjson::kObjectType);
		object_stack.push(value);
		size_t stacksize = object_stack.size();
		LOGV("begin_object: %zu\n", stacksize);
		return value;
	}

	rapidjson::Value* get_top()
	{
		return object_stack.top();
	}

	void end_object()
	{
		object_stack.pop();
		size_t stacksize = object_stack.size();
		LOGV("end_object: %zu\n", stacksize);
	}


	template <class T>
	void begin_property(const ClassProperty<T>& property)
	{
		LOGV("BEGIN property '%s', address: %p\n", property.name, &property.ref);
		rapidjson::Value* nextprop = begin_object();

		const reflection::TypeInfoCategory category = reflection::TypeCategory<T>::value;
		assert(category != reflection::TypeInfo_Invalid);

		if (category == reflection::TypeInfo_Class)
		{
			LOGV("-> subobject!\n");
			nextprop->SetObject();
		}
	}

	template <class T>
	void end_property(const ClassProperty<T>& property)
	{
		LOGV("END property '%s', address: %p\n", property.name, &property.ref);
		rapidjson::Value* value = object_stack.top();
		end_object();

		rapidjson::Value* root = object_stack.top();
		assert(root != &doc);

		root->AddMember(rapidjson::StringRef(property.name), *value, doc.GetAllocator());
	}

	template <class Archive, class T>
	void begin_class(Archive& ar, T& value)
	{
//		const reflection::TypeInfo* info = value.get_type_info();
		const reflection::TypeInfo* info = reflection::get_type_info<T>();
		assert(info);

		LOGV("begin class: %s\n", info->type_identifier());

		rapidjson::Value* node = get_top();
		node->SetObject();
		assert(node->IsObject());
	}

	template <class Archive, class T>
	void end_class(Archive& ar, T& value)
	{
		const reflection::TypeInfo* info = reflection::get_type_info<T>();
		assert(info);

		LOGV("end class: %s\n", info->type_identifier());
	}

	template <class Archive, class T>
	void save(Archive& ar, T value)
	{
		LOGV("should save something\n");
		value.serialize(ar, 1);
	}

	template <class Archive, class T>
	void save(Archive& ar, const T& value)
	{
		// Couldn't deduce the type.
		assert(0);
	}

	template <class Archive>
	void save(Archive& ar, const int& value)
	{
		rapidjson::Value* prop = object_stack.top();
		assert(value);
		prop->SetInt(value);
	}

	template <class Archive>
	void save(Archive& ar, const float& value)
	{
		rapidjson::Value* prop = object_stack.top();
		assert(prop);
		prop->SetDouble(value);
	}

	template <class Archive>
	void save (Archive& ar, const double& value)
	{
		rapidjson::Value* prop = object_stack.top();
		assert(prop);
		prop->SetDouble(value);
	}

	template <class Archive>
	void save(Archive& ar, const unsigned long& value)
	{
		rapidjson::Value* prop = object_stack.top();
		assert(prop);
		prop->SetUint64(value);
	}

	template <class Archive>
	void save(Archive& ar, const long& value)
	{
		rapidjson::Value* prop = object_stack.top();
		assert(prop);
		prop->SetInt64(value);
	}

	template <class Archive>
	void save(Archive& ar, const bool& value)
	{
		rapidjson::Value* prop = object_stack.top();
		assert(prop);
		prop->SetBool(value);
	}

	template <class Archive>
	void save(Archive& ar, const char* value)
	{
		rapidjson::Value* prop = object_stack.top();
		assert(prop);
		prop->SetString(value, core::str::len(value));
	}

	template <class Archive, class T>
	void save(Archive& ar, const Array<T>& value)
	{
		assert(0);
//		rapidjson::Value value;
//		size_t array_size = property.ref.size();
//		ar & make_class_property("size", array_size);
//
//		for (auto& item : property.ref)
//			ar << item;
//
//		return value;
	}
};


// declaration
template <class T, class... P0toN>
struct is_one_of;

// case when no matches due to empty list
template <class T>
struct is_one_of<T> : std::false_type {};

// case which matches the first item
template <class T, class... P1toN>
struct is_one_of<T, T, P1toN...> : std::true_type {};

// specialization which recognizes mismatch; calls recursively
template <class T, class P0, class... P1toN>
struct is_one_of<T, P0, P1toN...> : is_one_of<T, P1toN...> {};

// usage:
//	bool istype = is_one_of<int, float, size_t, double, int>::value;


class JsonReader : public Reader<JsonReader>
{
public:

	rapidjson::Document doc;

	JsonReader(const char* json)
	{
		doc.Parse(json);
		assert(!doc.HasParseError());
	}



	template <class T>
	void read_property(const reflection::ClassProperty<T>& property)
	{
		LOGV("READ property '%s', address: %p\n", property.name, &property.ref);
		(*instance()) & property.ref;
	}








	template <class Archive, class T>
	void save(Archive& ar, T value)
	{
		LOGV("should save something\n");
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
//		rapidjson::Value val;
//		val.SetInt(value);
//		doc.AddMember("value", val, doc.GetAllocator());
		if (doc.HasMember("value"))
		{
			rapidjson::Value& v = doc["value"];
			assert(v.IsInt());
			value = v.GetInt();
		}
	}

	template <class Archive>
	void save(Archive& ar, float& value)
	{
//		fl_value = value;
	}

	template <class Archive>
	void save(Archive& ar, bool& value)
	{
//		val = value;
	}

	template <class Archive>
	void save(Archive& ar, char& value)
	{

	}

	template <class Archive>
	void save(Archive& ar, char* value)
	{
		save(ar, (const char*)value);
	}

	template <class Archive>
	void save(Archive& ar, const char* value)
	{
//		rapidjson::Value v;
//		v.SetString(value, core::str::len(value));
//		doc.AddMember("value", v, doc.GetAllocator());
	}

	template <class Archive>
	void save(Archive& ar, size_t& value)
	{
	}


};

//#define MYCUSTOMCLASS_EXTERNAL_SERIALIZER

struct MyCustomClass
{
	MyCustomClass() :
		temperature(0)
	{
	}

	TYPEINFO_DECLARE_CLASS(MyCustomClass);

	int temperature;
	glm::vec3 pos;

#ifndef MYCUSTOMCLASS_EXTERNAL_SERIALIZER
	template <class Archive>
	void serialize(Archive& ar, size_t version)
	{
		LOGV("MyCustomClass.serialize\n");
		ar << TYPEINFO_PROPERTY(temperature);
		ar << TYPEINFO_PROPERTY(pos);
	}
#endif
};

#define SERIALIZATION_REGISTER_TYPE(T)\
	TYPEINFO_REGISTER_TYPE_INFO(T);\
	TYPEINFO_REGISTER_TYPE_CATEGORY(T, TypeInfo_Class);\
	TYPEINFO_REGISTER_TYPE_NAME(T, T)

SERIALIZATION_REGISTER_TYPE(MyCustomClass);


#ifdef MYCUSTOMCLASS_EXTERNAL_SERIALIZER
template <class Archive>
void serialize(Archive& ar, MyCustomClass& value)
{
	LOGV("serialize MyCustomClass (external)\n");
	ar << value.temperature;
	ar << value.pos;
}

template <>
struct SerializerTypeSelector<MyCustomClass>
{
	static constexpr SerializerType value = SerializerTypeExternal;
};

template <>
struct SerializerTypeSelector<MyCustomClass*>
{
	static constexpr SerializerType value = SerializerTypeExternal;
};

template <>
struct SerializerTypeSelector<MyCustomClass&>
{
	static constexpr SerializerType value = SerializerTypeExternal;
};
#endif


template <class T>
void dunk(T && head)
{
	// prologue
	LOGV("type: %s\n", reflection::TypeIdentifier<T>::get_type_identifier());
		// serialize (head)
	// epilogue
}

template <class T, class ... Types>
void dunk(T && head, Types && ... tail)
{
	dunk(head);
	dunk(tail ... );
}




//TYPEINFO_REGISTER_TYPE_INFO(glm::vec3);
//TYPEINFO_REGISTER_TYPE_CATEGORY(MyCustomClass, TypeInfo_Class);
//TYPEINFO_REGISTER_TYPE_NAME(MyCustomClass, MyCustomClass);


SERIALIZATION_REGISTER_TYPE_EXTERNAL(glm::vec3);
TYPEINFO_REGISTER_TYPE_NAME(glm::vec3, glm::vec3);
TYPEINFO_REGISTER_TYPE_CATEGORY(glm::vec3, TypeInfo_Class);



template <class Archive>
void serialize(Archive& ar, const glm::vec3& value)
{
	LOGV("serialize glm::vec3 (external)\n");
	ar << make_class_property("x", value.x, TYPEINFO_OFFSET(value, x));
	ar << make_class_property("y", value.y, TYPEINFO_OFFSET(value, y));
	ar << make_class_property("z", value.z, TYPEINFO_OFFSET(value, z));

	LOGV("value.x: %u\n", TYPEINFO_OFFSET(value, x));
	LOGV("value.y: %u\n", TYPEINFO_OFFSET(value, y));
	LOGV("value.z: %u\n", TYPEINFO_OFFSET(value, z));


//	ar & TYPEINFO_PROPERTY(temperature);
}

void test_rapidjson()
{
//	const char* json = R"(
//		{
//			"name": "adam",
//			"value": 3.25
//		}
//	)";

//	const int a = 30;
//	float* b;
//	char c;
//	dunk(a, b, c);

	using namespace rapidjson;

	Document doc;
//	Document::AllocatorType& allocator = doc.GetAllocator();


//	doc.Parse(json);
//	assert(!doc.HasParseError());
//
//	if (doc.HasMember("groups"))
//	{
//		Value& name = doc["groups"];
//		assert(!name.IsNull());
//	}

//	doc.SetObject();
//	Value value;
//	value.SetInt(30);
//	doc.AddMember("value", value, allocator);
//	StringBuffer buffer;
//	WriterType writer(buffer);
//	doc.Accept(writer);
//	LOGV("%s\n", buffer.GetString());



	int integer = 123;
	float vals[] = {0.0f, 1.2f, 3.2f, 25.452f};
	MyCustomClass klass;

	JsonWriter jw;
	int value = 30;
	int temp = 75;
	float precision = 7.25f;
	size_t size = 0xffffffff;
	size_t max_size = 0xffffffffffffffff;
	bool nope = false;
	const char name[] = "adam";
	char* pepper = new char[12];
	memset(pepper, 0, 12);
	strcpy(pepper, "pepper");

	int listo[4] = {0, 1, 2, 3};

	Array<int> arr;
	arr.push_back(12);



	klass.temperature = 42;
	klass.pos = glm::vec3(3.2f, 1.25f, 7.0f);

	static_assert(
		reflection::TypeCategory<reflection::ClassProperty<int>>::value == reflection::TypeInfo_Property,
		"bah"
	);

//	jw << integer;
//	jw << vals;
	jw << klass;
//	jw << &klass;
//	jw << listo;
//	jw << value;
//	jw << temp;
//	jw << precision;
//	jw << size;
//	jw << max_size;
//	jw << nope;
//	jw << name;
//	jw << pepper;


//	jw << make_class_property("value", value);
//	jw << make_class_property("temp", temp);
//	jw << make_class_property("precision", precision);
//	jw << make_class_property("size", size);
//	jw << make_class_property("max_size", max_size);
//	jw << make_class_property("nope", nope);
//	jw << make_class_property("name", name);
//	jw << make_class_property("pepper", pepper);
//	jw << make_class_property("values", arr);


	jw.generate_document();
	const char* json = jw.get_string();
	LOGV("buffer: %s\n", json);

	delete [] pepper;


//	JsonReader reader(json);
//
//	value = -1;
//
//	reader >> value;





//	MyCustomClass output;
//	JsonReader reader(json);
//	reader >> output;

//	Value obj(kArrayType);
//	doc.AddMember("groups", obj, allocator);

#if 0
	typedef rapidjson::Writer<StringBuffer> WriterType;
#else
	typedef rapidjson::PrettyWriter<StringBuffer> WriterType;
#endif


}







static void free_function(int param)
{
	LOGV("free_function called with: %i\n", param);
}

class MyClass
{
public:
	void member_function(int param)
	{
		LOGV("member_function called with: %i\n", param);
	}

	void test_value(int param) const
	{
		LOGV("test_value value is: %i, %i\n", value, param);
	}

	static void static_member_function(int param)
	{
		LOGV("static_member_function called with: %i\n", param);
	}

private:
	int value;
};


class AnotherClass
{
public:
	void dispatch_test(int value)
	{
		LOGV("dispatch_test: %i\n", value);
	}

private:
	// whatever
};




template <class... Args>
struct temp
{
	std::function<void (Args...)> xt;

	void execute(Args... args)
	{
		xt(args...);
	}
};

#if 0
	// sidetracked with move constructors
	Movable a("name");
	LOGV("value is: %s\n", a.data);

	// move constructor
	Movable x(std::move(a));
	LOGV("value is: %s\n", x.data);
#endif

struct Movable
{
	int data_size;
	char* data;

	Movable(const char* input)
	{
		data_size = core::str::len(input);
		data = new char[data_size+1];
		data[data_size] = 0;

		core::str::copy(data, input, data_size);
	}

	~Movable()
	{
		data_size = 0;
		delete [] data;
		data = nullptr;
	}

	// move constructor
	Movable(Movable&& other)
		: data_size(0)
		, data(nullptr)
	{
		data_size = other.data_size;
		data = other.data;

		other.data_size = 0;
		other.data = nullptr;
	}

	// move assignment operator
	Movable& operator=(Movable&& other)
	{
		// 1. release any resources this owns
		// 2. pilfer other's resources
		// 3. reset other's to default
		// 4. return *this

		if (this != &other)
		{
			if (data)
			{
				delete [] data;
				data_size = 0;
				data = nullptr;
			}

			data_size = other.data_size;
			data = other.data;

			other.data_size = 0;
			other.data = nullptr;
		}

		return *this;
	}
};


namespace reflection_test
{
	class Base
	{
	public:
		void foo()
		{
			LOGV("foo called\n");
		}


		template <class Archive>
		void reflect_members(Archive& ar, size_t version)
		{

		}
	private:

		int private_impl;
		size_t xyz;


		int serializable_value;

		/*

		Reflection<int> serializable_value;
		Reflection(int, serializable_value);
		BEGIN_REFLECTION(Base)
			REFLECTION(int, serializable_value)
		END_REFLECTION()

		*/
	};
}


void test_reflection()
{
	using namespace reflection_test;



}


int main(int, char**)
{
	// platform startup is needed to install the platform log handler
	gemini::core_startup();
	platform::startup();

	unittest::UnitTest::execute();
	//test_rapidjson();
	test_reflection();

	platform::shutdown();
	gemini::core_shutdown();


	return 0;
}

#if 0
#define UNITTEST(name) void test_##name(UnitTestCategory& category)

UNITTEST(StackString)
{

}
#endif

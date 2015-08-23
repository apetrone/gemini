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

	abc.pop_back();

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


	Array<int> rd;
	rd.push_back(30);
	rd.push_back(60);
	rd.push_back(90);
	rd.resize(4, 0);

	TEST_VERIFY(rd[0] == 30, resize_default_keep_existing);
	TEST_VERIFY(rd[3] == 0, resize_default_set);
	// test resize with default value
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
	TEST_VERIFY(z == 72, hash_with_global_allocator);
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
//		fprintf(stdout, "serialize for test: %zu\n", offsetof(Test, value));
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
		fprintf(stdout, "serialize for test2 \n");

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
		fprintf(stdout, "serialize for test3\n");
//		ar & TYPEINFO_PROPERTY(test3_value);
		ar & test3_value;
	}
}; // Test3

TYPEINFO_REGISTER_TYPE_NAME(Test3, Test3);
TYPEINFO_REGISTER_TYPE_INFO(Test3);
TYPEINFO_REGISTER_TYPE_BASE(Test3, Test2);


void test_serialization()
{
	// The impetus behind writing serialization has to do with the UI code
	// I would like to define a layout in JSON and then read that back in
	// to create the interface panels.

	// There's a bit of a learning curve, but it's healthy.
	// I will start simple to begin with and serialize POD types first.
	// Then, I'll move on and create data structures and try to clean up
	// as I go.

	TestArchive writer;
	TestReader reader;

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
	fprintf(stdout, "type is: '%s'\n", typeinfo->type_identifier());
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
	fprintf(stdout, "t2i is '%s'\n", t2i->type_identifier());

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
		fprintf(stdout, "-> %s\n", curr->type_identifier());
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

	std::stack<rapidjson::Value> object_stack;
public:

	JsonWriter()
	{
		doc.SetObject();
	}

	~JsonWriter()
	{
		generate_document();
	}

	void generate_document()
	{
		buffer.Clear();
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
	}

	const char* get_string()
	{
		return buffer.GetString();
	}

	template <class Archive, class T>
	rapidjson::Value get_value(Archive& ar, const ClassProperty<T>& property)
	{
		rapidjson::Value value;
		assert(0);
		return value;
	}

	template <class Archive>
	rapidjson::Value get_value(Archive& ar, const ClassProperty<int>& property)
	{
		rapidjson::Value value(rapidjson::kNumberType);
		value.SetInt(property.ref);
		return value;
	}

	template <class Archive>
	rapidjson::Value get_value(Archive& ar, const ClassProperty<float>& property)
	{
		rapidjson::Value value(rapidjson::kNumberType);
		value.SetInt(property.ref);
		return value;
	}

	template <class Archive>
	rapidjson::Value get_value(Archive& ar, const ClassProperty<unsigned long>& property)
	{
		rapidjson::Value value(rapidjson::kNumberType);
		value.SetInt64(property.ref);
		return value;
	}

	template <class Archive>
	rapidjson::Value get_value(Archive& ar, const ClassProperty<bool>& property)
	{
		rapidjson::Value value;
		value.SetBool(property.ref);
		return value;
	}

	template <class Archive>
	rapidjson::Value get_value(Archive& ar, const ClassProperty<const char*>& property)
	{
		rapidjson::Value value;
		value.SetString(property.ref, core::str::len(property.ref));
		return value;
	}

	template <class Archive>
	rapidjson::Value get_value(Archive& ar, const ClassProperty<char*>& property)
	{
		rapidjson::Value value;
		value.SetString(property.ref, core::str::len(property.ref));
		return value;
	}

	template <class Archive, class T>
	rapidjson::Value get_value(Archive& ar, const ClassProperty< Array<T> >& property)
	{
		rapidjson::Value value;

		size_t array_size = property.ref.size();
		ar & make_class_property("size", array_size);

		for (auto& item : property.ref)
			ar << item;

		return value;
	}

	template <class T>
	void save_property(const ClassProperty<T>& property)
	{
		rapidjson::Value val = get_value(*instance(), property);
		doc.AddMember(rapidjson::StringRef(property.name), val, doc.GetAllocator());
	}

	template <class T>
	void write_property(const ClassProperty<T>& property)
	{
		fprintf(stdout, "WRITE property '%s', address: %p\n", property.name, &property.ref);
		instance()->save_property<T>(property);
	}





	template <class Archive, class T>
	void save(Archive& ar, T value)
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
		rapidjson::Value val;
		val.SetInt(value);
		doc.AddMember("value", val, doc.GetAllocator());
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
		rapidjson::Value v;
		v.SetString(value, core::str::len(value));
		doc.AddMember("value", v, doc.GetAllocator());
	}
};

void test_rapidjson()
{
//	const char* json = R"(
//		{
//			"name": "adam",
//			"value": 3.25
//		}
//	)";

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
//	fprintf(stdout, "%s\n", buffer.GetString());





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


	Array<int> arr;
	arr.push_back(12);

	jw << value;
	jw << name;
	jw << pepper;
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
	fprintf(stdout, "buffer: %s\n", jw.get_string());

	delete [] pepper;

//	Value obj(kArrayType);
//	doc.AddMember("groups", obj, allocator);

#if 0
	typedef rapidjson::Writer<StringBuffer> WriterType;
#else
	typedef rapidjson::PrettyWriter<StringBuffer> WriterType;
#endif


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
	test_serialization();
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

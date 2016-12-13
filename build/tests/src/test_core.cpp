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
#include <core/core.h>
#include <core/datastream.h>
#include <core/fixedarray.h>
#include <core/fixedsizequeue.h>
#include <core/hashset.h>
#include <core/mathlib.h>
#include <core/mem.h>
#include <core/linearfreelist.h>
#include <core/str.h>
#include <core/stackstring.h>
#include <core/threadsafequeue.h>
#include <core/typespec.h>
#include <core/util.h>

#include <platform/platform.h>

#include <vector>
#include <string>
#include <assert.h>


using namespace core;
using namespace core::util;

#if 0
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
		a.push_back(static_cast<int>(i+1));
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
		LOGV("value %i\n", value);
	}

	abc.erase(60);

	abc.pop_back();

	TEST_ASSERT(abc.size() == 1, size_after_erase);
	for (const int& value : abc)
	{
		LOGV("value %i\n", value);
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


	// test array copy
	{
		Array<int> temp = values;
		TEST_ASSERT(temp == values, copy_and_equality);
	}
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
// Delegate
// ---------------------------------------------------------------------
class SomeType
{
public:
	void	vv_foo()				{}
	void	vi_foo(int)				{}
	int		iv_foo()				{ return 42; }
	int		ii_foo(int x)			{ return x + 10; }
	int		iii_foo(int a, int b)	{ return (a + b); }
	void	vii_foo(int, int)		{}
};

static void vv_foo()				{}
static void vi_foo(int)				{}
static void vii_foo(int, int)		{}
static int iv_foo()					{ return 42; }
static int ii_foo(int x)			{ return x + 10; }
static int iii_foo(int a, int b)	{ return (a + b); }

UNITTEST(Delegate)
{
	using namespace gemini;
	SomeType st;

	Delegate<void()> bx = MAKE_STATIC_DELEGATE(void(), vv_foo);
	bx();

	Delegate<void()> a;
	a.bind<vv_foo>();
	a();
	a.bind<SomeType, &SomeType::vv_foo>(&st);
	a();

	Delegate<void(int)> b;
	b.bind<vi_foo>();
	b(42);
	b.bind<SomeType, &SomeType::vi_foo>(&st);
	b(41);

	Delegate<int()> c;
	c.bind<iv_foo>();
	int c_result = c();
	TEST_ASSERT(c_result == 42, delegate_int_return);
	c.bind<SomeType, &SomeType::iv_foo>(&st);
	c_result = c();
	TEST_ASSERT(c_result == 42, delegate_member_int_return);

	Delegate<void(int, int)> e;
	e.bind<vii_foo>();
	e(42, 41);
	e.bind<SomeType, &SomeType::vii_foo>(&st);
	e(30, 20);

	Delegate<int(int, int)> f;
	f.bind<iii_foo>();
	int f_result = f(10, 20);
	TEST_ASSERT(f_result == 30, delegate_int_sum_one);
	f.bind<SomeType, &SomeType::iii_foo>(&st);
	f_result = f(10, 20);
	TEST_ASSERT(f_result == 30, delegate_member_int_sum_one);

	Delegate<int(int)> g;
	g.bind<ii_foo>();
	int g_result = g(50);
	TEST_ASSERT(g_result == 60, delegate_int_sum_two);
	g.bind<SomeType, &SomeType::ii_foo>(&st);
	g_result = g(50);
	TEST_ASSERT(g_result == 60, delegate_member_int_sum_two);

	Delegate<int(int)> z = MAKE_MEMBER_DELEGATE(int(int), SomeType, &SomeType::ii_foo, &st);
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
// linear free list
// ---------------------------------------------------------------------
UNITTEST(linearfreelist)
{
	LinearFreeList<int> abc;
	LinearFreeList<int>::Handle value = abc.acquire();
	if (abc.is_valid(value))
	{
		int* beh = abc.from_handle(value);
		*beh = 73;

		// done with that, return it to the free list.
		abc.release(value);

		// try to acquire another (should be the same as beh was).
		value = abc.acquire();

		int* foo = abc.from_handle(value);
		*foo = 42;
		TEST_ASSERT(foo == beh, acquire_and_release);
	}

	size_t valid_handles = 0;
	for (size_t index = 0; index < abc.size(); ++index)
	{
		if (abc.is_valid(index))
		{
			int* p = abc.from_handle(index);
			LOGV("index: %i, ptr: %p %i\n", index, p, *p);
			++valid_handles;
		}
	}

	TEST_ASSERT(valid_handles == 1, is_valid);
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


TYPESPEC_REGISTER_POD(uint32_t);


class Test
{
public:
	TYPESPEC_DECLARE_CLASS_NOBASE(Test)

	virtual ~Test() {}

	virtual void speak()
	{
		LOGV("Test speak\n");
	}
};
TYPESPEC_REGISTER_CLASS(Test);


class DerivedTest : public Test
{
public:
	TYPESPEC_DECLARE_CLASS(DerivedTest, Test)

	virtual void speak() override
	{
		LOGV("DerivedTest speak\n");
	}
};
TYPESPEC_REGISTER_CLASS(DerivedTest);

template <class T>
struct MyTest
{
	static T* instance;
};

// template <>
// Test* MyTest<Test>::instance = &(Test());

// ---------------------------------------------------------------------
// typespec
// ---------------------------------------------------------------------
UNITTEST(typespec)
{
	size_t value = STRING_HASH32("hello");
	TEST_ASSERT(value == STRING_HASH32("hello"), string_hash32);



	LOGV("type of int is: %s\n", TypeSpecName<uint32_t>::value);
	LOGV("type of int is: %i\n", TypeSpecIdentifier<uint32_t>::value);
	LOGV("size of type uint32_t is %i\n", TypeSpecSize<uint32_t>::value);

	uint32_t b = 32;

	uint32_t* a = &b;

	Test klass;
	LOGV("size of Test = %i\n", sizeof(Test));
	LOGV("Test name = %s, identifier = %i\n", klass.typespec()->name(), klass.typespec()->identifier());
	if (klass.typespec()->identifier() == TYPESPEC_IDENTIFIER("Test"))
	{
		LOGV("type is a Test\n");
	}

	DerivedTest derived;
	LOGV("size of DerivedTest = %i\n", sizeof(DerivedTest));
	LOGV("DerivedTest name = %s, identifier = %i\n", derived.typespec()->name(), derived.typespec()->identifier());
	if (derived.typespec()->identifier() == TYPESPEC_IDENTIFIER("DerivedTest"))
	{
		LOGV("type is a DerivedTest\n");
	}


	if (STRING_HASH32("uint32_t") == typespec_identifier_from_value(a))
	{
		LOGV("a is a uint32_t\n");
	}

	DerivedTest second_derived;
	TEST_ASSERT(derived.typespec() == second_derived.typespec(), typespec);

	// const char* t = typespec_name_from_value(a);
	// assert(t);

//	if (is_same_type(a, "uint32_t"))
//	{
//		LOGV("types match!\n");
//	}
//	else
//	{
//		LOGV("types don't match\n");
//	}

	//uint32_t value = hash_string<5>("hello");
}

//template <class T>
//bool is_same_type(T* value, const char* type_name)
//{
//	const char* deduced_type_name = typespec_name_from_value(value);
//	return core::str::case_insensitive_compare(type_name, deduced_type_name, 0) == 0;
//}




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
#endif


// Anything that allocates memory should accept an allocator.
// Anything that deletes memory should accept an allocator.
// Any container should be able to accept an allocator.

// Be able to toggle between debug/release versions of allocator.
// Specify different allocation strategies

enum AllocatorType
{
	ALLOCATOR_STATIC,
	ALLOCATOR_TESTING,
	ALLOCATOR_FIXED
};

struct Allocator
{
	void* (*allocate)(Allocator*, size_t bytes, size_t alignment);
	void (*deallocate)(Allocator*, void* pointer);

	char* memory;
	size_t memory_size;
	AllocatorType type;
};

void* static_allocate(Allocator* allocator, size_t bytes, size_t alignment)
{
	return core::memory::aligned_malloc(bytes, alignment);
}

void static_deallocate(Allocator* allocator, void* pointer)
{
	core::memory::aligned_free(pointer);
}

void* null_allocate(Allocator* allocator, size_t bytes, size_t alignment)
{
	//return core::memory::aligned_malloc(bytes, alignment);
	return 0;
}

void null_deallocate(Allocator* allocator, void* pointer)
{
	core::memory::aligned_free(pointer);
}


Allocator static_allocator_create()
{
	Allocator sa;
	sa.allocate = static_allocate;
	sa.deallocate = static_deallocate;
	sa.memory = 0;
	sa.memory_size = 0;
	sa.type = ALLOCATOR_STATIC;
	return sa;
}

Allocator null_allocator_create()
{
	Allocator sa;
	sa.allocate = null_allocate;
	sa.deallocate = null_deallocate;
	sa.memory = 0;
	sa.memory_size = 0;
	sa.type = ALLOCATOR_TESTING;
	return sa;
}



#define DEBUG_MEMORY 1
#if DEBUG_MEMORY
#define ALLOCATE_MEMORY(allocator, bytes) mem_allocate(allocator, bytes, sizeof(void*), __FILE__, __LINE__)
#define DEALLOCATE_MEMORY(allocator, pointer) mem_deallocate(allocator, pointer, __FILE__, __LINE__)

void* mem_allocate(Allocator* allocator, size_t bytes, size_t alignment, const char* filename, int line)
{
	void* memory = allocator->allocate(allocator, bytes, alignment);
	LOGV("[+] %p %i @ %i | '%s':%i\n", memory, bytes, alignment, filename, line);
	return memory;
}

void mem_deallocate(Allocator* allocator, void* pointer, const char* filename, int line)
{
	allocator->deallocate(allocator, pointer);
	LOGV("[-] %p | '%s':%i\n", pointer, filename, line);
}
#else
#define ALLOCATE_MEMORY(allocator, bytes) mem_allocate(allocator, bytes, sizeof(void*))
#define DEALLOCATE_MEMORY(allocator, pointer) mem_deallocate(allocator, pointer)

void* mem_allocate(Allocator* allocator, size_t bytes, size_t alignment)
{
	return allocator->allocate(allocator, bytes, alignment);
}

void mem_deallocate(Allocator* allocator, void* pointer)
{
	allocator->deallocate(allocator, pointer);
}
#endif


struct TestDevice
{
	size_t index;
};


TestDevice* create_device(Allocator* allocator)
{
	void* mem = ALLOCATE_MEMORY(allocator, sizeof(TestDevice));
	char* memory = reinterpret_cast<char*>(mem);

	new (memory) TestDevice();

	return reinterpret_cast<TestDevice*>(memory);
}

void destroy_device(Allocator* allocator, TestDevice* device)
{
	device->~TestDevice();
	DEALLOCATE_MEMORY(allocator, device);
}



int main(int, char**)
{
	gemini::core_startup();
	//unittest::UnitTest::execute();

	Allocator sa = static_allocator_create();
	//Allocator sa = null_allocator_create();

	TestDevice* test = create_device(&sa);
	test->index = 0;

	destroy_device(&sa, test);




	gemini::core_shutdown();
	return 0;
}

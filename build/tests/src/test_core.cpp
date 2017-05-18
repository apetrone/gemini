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
#include <core/freelist.h>
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
#include <platform/directory_monitor.h>

#include <vector>
#include <string>
#include <assert.h>


using namespace core;
using namespace core::util;

using namespace gemini;


class TestClassNoDefaultConstructor
{
public:
	TestClassNoDefaultConstructor(int& index)
		: _index(index)
	{
	}

	TestClassNoDefaultConstructor& operator=(const TestClassNoDefaultConstructor& other)
	{
		_index = other._index;
		return *this;
	}

	int get_index() const
	{
		return _index;
	}

private:
	int& _index;
}; // TestClassNoDefaultConstructor

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
	Allocator default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	Array<int> a(default_allocator);

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



	Array<int> abc(default_allocator, 6);
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

	Array<CustomType> b(default_allocator, 4);
	TEST_ASSERT(_local_counter == 4, constructor_called_n_times);
	b.clear();

	TEST_ASSERT(_local_counter == 0, destructor_called_n_times);

	// test resize with default value
	Array<int> rd(default_allocator);
	rd.push_back(30);
	rd.push_back(60);
	rd.push_back(90);
	rd.resize(4, 0);

	TEST_ASSERT(rd[0] == 30, resize_default_keep_existing);
	TEST_ASSERT(rd[3] == 0, resize_default_set);


	// test qsort
	Array<int> values(default_allocator);
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

	// test array resizing
	{
		Array<int> test(default_allocator, 0);

		test.resize(1);
		test[0] = 72;

		test.resize(2);
		test[1] = 16;

		TEST_ASSERT_EQUALS(test[0], 72);
		TEST_ASSERT_EQUALS(test[1], 16);

		test.push_back(24);
		TEST_ASSERT_EQUALS(test[2], 24);

		test.resize(4);
		test.push_back(120);
		TEST_ASSERT_EQUALS(test[4], 120);
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
// directory monitor
// ---------------------------------------------------------------------
void monitor_triggered(MonitorHandle handle, MonitorAction action, const platform::PathString& path)
{
	LOGV("monitor_triggered with: %s\n", path());
}

UNITTEST(directory_monitor)
{
	Allocator allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	int32_t startup_result = directory_monitor_startup(allocator);
	TEST_ASSERT_EQUALS(startup_result, 0);

	MonitorDelegate delegate;
	delegate.bind<monitor_triggered>();

	platform::PathString directory = platform::get_program_directory()();
	LOGV("adding watch for directory: \"%s\"\n", directory());

	MonitorHandle handle0 = directory_monitor_add(directory(), delegate);

	TEST_ASSERT_TRUE(handle0 > 0);

	// TODO: Figure out a good way to test this.
	// size_t index = 0;
	// LOGV("waiting for file changes...\n");

	// while (index < 0xFFFFFF)
	// {
	// 	directory_monitor_update();
	// 	++index;
	// }

	directory_monitor_remove(handle0);

	directory_monitor_shutdown();
}


// ---------------------------------------------------------------------
// FixedArray
// ---------------------------------------------------------------------
UNITTEST(FixedArray)
{
	gemini::Allocator default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	FixedArray<int> int_array(default_allocator);
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


	// allocate items that don't have a default constructor
	FixedArray<TestClassNoDefaultConstructor> items(default_allocator);
	int test_index = 127;

	TestClassNoDefaultConstructor test_default(test_index);
	items.allocate(32, test_default);

	TEST_ASSERT_EQUALS(items[0].get_index(), 127);
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
	Allocator default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	HashSet<std::string, int> dict(default_allocator);
	dict["first"] = 1;
	TEST_ASSERT_EQUALS(dict.size(), 1);
	dict["second"] = 2;
	dict["third"] = 3;
	dict["fourth"] = 4;

	TEST_ASSERT_EQUALS(dict.size(), 4);

	TEST_ASSERT_TRUE(dict.has_key("third"));

	dict.insert(std::pair<std::string, int>("fifth", 5));
	TEST_ASSERT_EQUALS(dict.size(), 5);

	dict.clear();
	TEST_ASSERT_EQUALS(dict.size(), 0);

	HashSet<std::string, void*> custom_capacity(default_allocator, 4096);
	TEST_ASSERT_EQUALS(custom_capacity.capacity(), 4096);

	int second = dict.get("second");
	TEST_ASSERT_EQUALS(2, second);

	bool has_two = false;
	HashSet<int, int> repop_test(default_allocator, 5);
	repop_test[0] = 0;
	repop_test[1] = 1;
	repop_test[2] = 2;
	has_two = repop_test.has_key(2);
	repop_test[3] = 3;
	has_two = repop_test.has_key(2);
	repop_test[4] = 4;
	has_two = repop_test.has_key(2);

	HashSet<int, int, core::util::hash<int>> test(default_allocator, 32, 2);
	test.insert(HashSet<int, int>::value_type(30, 72));
	int z = test[30];
	TEST_ASSERT_EQUALS(z, 72);


	// test insertion
	test[42] = 67;
	TEST_ASSERT_EQUALS(test[42], 67);
	TEST_ASSERT_TRUE(test.has_key(42));

	// test removal
	test.remove(42);
	TEST_ASSERT_FALSE(test.has_key(42));
}


// ---------------------------------------------------------------------
// CircularBuffer
// ---------------------------------------------------------------------
UNITTEST(CircularBuffer)
{
	Allocator default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	CircularBuffer<int, 3> cb(default_allocator);
	int& a = cb.next();
	a = 30;

	int& b = cb.next();
	b = 60;

	int& c = cb.next();
	c = 90;

	int d = cb.next();
	TEST_ASSERT(d == a, circular_buffer);
}


UNITTEST(freelist)
{
	struct MyStructNoDefaultCtor
	{
		int x;

		MyStructNoDefaultCtor(int value)
			: x(value)
		{
		}
	};

	Allocator allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);

	Freelist<int> list1(allocator);
	Freelist<int>::Handle handle1, handle2, handle3;
	handle1 = list1.acquire();
	handle2 = list1.acquire();
	handle3 = list1.acquire();

	list1.set(handle1, 20);
	list1.set(handle2, 40);
	list1.set(handle3, 60);

	TEST_ASSERT_EQUALS(list1.size(), 3);

	list1.release(handle2);

	int v1 = list1.from_handle(handle1);
	TEST_ASSERT_EQUALS(v1, 20);

	int v2 = list1.from_handle(handle3);
	TEST_ASSERT_EQUALS(v2, 60);

	Freelist<MyStructNoDefaultCtor*> list2(allocator);
	Freelist<MyStructNoDefaultCtor*>::Handle handle = list2.acquire();
	list2.set(handle, new MyStructNoDefaultCtor(72));

	Freelist<MyStructNoDefaultCtor*>::Iterator iter;
	for (iter = list2.begin(); iter != list2.end(); ++iter)
	{
		MyStructNoDefaultCtor* instance = iter.data();
		delete instance;
	}
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
struct TestDevice
{
	size_t index;
}; // TestDevice


struct TestStruct
{
	TestStruct(int alpha, int beta, int gamma = 0)
	{
		a = alpha;
		b = beta;
		c = gamma;
	}

	size_t index;
	int a;
	int b;
	int c;
}; // TestStruct


#pragma pack(push, 16)
struct PLATFORM_ALIGN(16) AlignedStructTest
{
	float one[4];
	float two[4];
	float three[3];
	uint32_t padding;
}; // AlignedStructTest
#pragma pack(pop)


UNITTEST(memory)
{
	// 1. Sequential Allocation and Deallocation
	Allocator sa = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	TestDevice* test = MEMORY2_NEW(sa, TestDevice);
	TEST_ASSERT_TRUE(test != nullptr);

	test->index = 42;
	TEST_ASSERT_EQUALS(test->index, 42);

	MEMORY2_DELETE(sa, test);
	test = nullptr;


	// 2. Allocate three items and delete from the middle.
	TestDevice* one = MEMORY2_NEW(sa, TestDevice);
	one->index = 1;

	TestDevice* two = MEMORY2_NEW(sa, TestDevice);
	two->index = 2;

	TestDevice* three = MEMORY2_NEW(sa, TestDevice);
	three->index = 3;

	MEMORY2_DELETE(sa, two);
	MEMORY2_DELETE(sa, one);
	MEMORY2_DELETE(sa, three);

	// 3. Test arrays
	TestDevice* devices = MEMORY2_NEW_ARRAY(sa, TestDevice, 64);
	TEST_ASSERT_TRUE(devices != nullptr);
	MEMORY2_DELETE_ARRAY(sa, devices);
}

UNITTEST(memory_allocator_linear)
{
	// 1. Test linear allocator new.
	char buffer[256];
	Allocator linear = memory_allocator_linear(MEMORY_ZONE_DEFAULT, buffer, 256);
	TestDevice* device = MEMORY2_NEW(linear, TestDevice);
	TEST_ASSERT_TRUE(device != nullptr);
	MEMORY2_DELETE(linear, device);

	// 2. Test linear allocator with arrays
	Allocator s2 = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	TestDevice* items = MEMORY2_NEW_ARRAY(s2, TestDevice, 64);
	MEMORY2_DELETE_ARRAY(s2, items);
	int* items2 = MEMORY2_NEW_ARRAY(s2, int, 8);
	for (size_t index = 0; index < 8; ++index)
	{
		items2[index] = int(index * 2);
	}
	MEMORY2_DELETE_ARRAY(s2, items2);
}


UNITTEST(memory_alignment)
{
	const size_t size = sizeof(AlignedStructTest);
	TEST_ASSERT_EQUALS(size, 48);

	const size_t alignment = alignof(AlignedStructTest);
	TEST_ASSERT_EQUALS(alignment, 16);

	// allocate the test struct and ensure its alignment.
	Allocator allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
	LOGV("trying to allocate to alignment of %i\n", alignof(AlignedStructTest));
	AlignedStructTest* value = MEMORY2_NEW(allocator, AlignedStructTest);
	LOGV("value is %p\n", value);
	TEST_ASSERT_TRUE(memory_is_aligned(value, 16));
	memset(value, 0, sizeof(AlignedStructTest));
	value->two[0] = 0.25f;
	value->two[1] = 0.35f;
	value->two[2] = 0.45f;
	value->two[3] = 0.55f;
	MEMORY2_DELETE(allocator, value);
}


UNITTEST(memory_arithmetic)
{
	unsigned char* mem = (unsigned char*)0x800;

	// start off aligned to an 8-byte boundary.
	TEST_ASSERT_TRUE(memory_is_aligned(mem, 8));

	// simulate allocation and pointer increment.
	mem += 56;
	TEST_ASSERT_TRUE(memory_is_aligned(mem, 8));
	TEST_ASSERT_FALSE(memory_is_aligned(mem, 16));

	// simulate a second allocation
	mem += 80;
	TEST_ASSERT_TRUE(memory_is_aligned(mem, 8));
	TEST_ASSERT_FALSE(memory_is_aligned(mem, 16));

	// align memory to 16-byte boundary.
	mem = (unsigned char*)memory_force_alignment(mem, 16);
	TEST_ASSERT_TRUE(memory_is_aligned(mem, 16));
}


UNITTEST(memory_static)
{
	// reserve static memory for 1 test device.
	StaticMemory<TestDevice> device_memory;
	TestDevice* test0 = memory_static_allocate(device_memory);
	TEST_ASSERT_TRUE(test0 != nullptr);
	test0->index = 72;
	TEST_ASSERT_EQUALS(test0->index, 72);

	StaticMemory<TestDevice> memA;
	TestDevice* test_device = memory_static_allocate(memA);
	TEST_ASSERT_TRUE(test_device != nullptr);

	StaticMemory<TestStruct> memB;
	TestStruct* test_value = memory_static_allocate(memB, 32, 16, 42);
	TEST_ASSERT_TRUE(test_value != nullptr);
	TEST_ASSERT_EQUALS(test_value->a, 32);
	TEST_ASSERT_EQUALS(test_value->b, 16);
	TEST_ASSERT_EQUALS(test_value->c, 42);
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
	Allocator default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);

	LinearFreeList<int> abc(default_allocator);
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

	// stress test
	for (size_t index = 0; index < 10000; ++index)
	{
		LinearFreeList<int>::Handle handle = abc.acquire();
		if ((index % 10) == 0)
		{
			// delete every 10th item.
			abc.release(handle);
		}
	}

	LinearFreeList<int>::Handle last_handle = abc.acquire();
	TEST_ASSERT_EQUALS(last_handle, 9001);

	abc.clear();
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
// resizable_memory_stream
// ---------------------------------------------------------------------
UNITTEST(resizable_memory_stream)
{
	gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
	ResizableMemoryStream mem(allocator);

	const float INITIAL_VALUE0 = 3.27f;
	const uint64_t INITIAL_VALUE1 = 1000483;
	const uint8_t INITIAL_VALUE2 = 127;

	float value = INITIAL_VALUE0;
	uint64_t test = INITIAL_VALUE1;
	uint8_t test2 = INITIAL_VALUE2;
	mem.reserve(512);

	mem.write(&value, sizeof(float));
	mem.write(&test, sizeof(uint64_t));
	mem.write(&test2, sizeof(uint8_t));

	mem.rewind();

	value = 0;
	test = 0;
	test2 = 0;

	mem.read(&value, sizeof(float));
	mem.read(&test, sizeof(uint64_t));
	mem.read(&test2, sizeof(uint8_t));

	TEST_ASSERT_EQUALS(INITIAL_VALUE0, value);
	TEST_ASSERT_EQUALS(INITIAL_VALUE1, test);
	TEST_ASSERT_EQUALS(INITIAL_VALUE2, test2);
}

// ---------------------------------------------------------------------
// serialization
// ---------------------------------------------------------------------
UNITTEST(serialization)
{
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


int main(int, char**)
{
	gemini::core_startup();
	unittest::UnitTest::execute();
	gemini::core_shutdown();
	return 0;
}

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


int main(int, char**)
{
	// platform startup is needed to install the platform log handler
	gemini::core_startup();
	platform::startup();

	unittest::UnitTest::execute();

	platform::shutdown();
	gemini::core_shutdown();

	return 0;
}

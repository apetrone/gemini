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
#include <core/color.h>
#include <core/datastream.h>


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
// Color
// ---------------------------------------------------------------------
void test_color()
{
	TEST_CATEGORY(Color);

	Color red(255, 0, 0, 255);
	
	Color temp;
	float red_float[4] = {1.0f, 0.0f, 0.0f, 1.0f};
	temp = Color::fromFloatPointer(red_float, 4);
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


int main(int, char**)
{
	test_argumentparser();
	test_color();
	test_datastream();
	return 0;
}
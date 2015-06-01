#include "unit_test.h"

#include <core/argumentparser.h>
#include <core/color.h>
#include <vector>
#include <string>
#include <assert.h>
#include <stdio.h>

using namespace core;


// ---------------------------------------------------------------------
// argument parser
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

int main(int, char**)
{
	test_argumentparser();
	test_color();
	return 0;
}
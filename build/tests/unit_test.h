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

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

// This system needs to be replaced with something a little
// more robust. This cannot handle tests within scope properly
// and causes an infinite loop because the linked list will be
// created with two identical addresses in this case.

struct UnitTest
{
	const char* name;
	int line_number;
	UnitTest* next;
	bool failed;

	UnitTest(const char* test_name, int line, bool failed) :
		name(test_name),
		line_number(line),
		failed(failed),
		next(nullptr)
	{
	}
};

struct UnitTestCategory
{
	const char* name;
	uint32_t failed_tests;
	uint32_t total_tests;
	UnitTest* root;

	UnitTestCategory(const char* suite_name) :
		name(suite_name),
		failed_tests(0),
		total_tests(0),
		root(nullptr)
	{
	}

	~UnitTestCategory()
	{
		if (root)
		{
			UnitTest* current = reverse_list(root);
			while(current)
			{
				if (current->failed)
				{
					fprintf(stdout, "FAILED '%s.%s' (line: %i)\n", name, current->name, current->line_number);
					++failed_tests;
				}
				else
				{
					fprintf(stdout, "PASSED '%s.%s'\n", name, current->name);
				}
				current = current->next;
			}
		}

		uint32_t passed_tests = (total_tests-failed_tests);
		fprintf(stdout, "* %s: %u / %u (%2.2f%%) passed\n\n", name, passed_tests, total_tests, (passed_tests/(float)total_tests)*100.0f);
	}

	UnitTest* reverse_list(UnitTest* root)
	{
		UnitTest* output = nullptr;
		UnitTest* curr = root;
		while(curr)
		{
			// we traverse the list starting at root
			// and simultaneously build the new root with output.
			UnitTest* next = curr->next;
			curr->next = output;
			output = curr;
			curr = next;
		}


		return output;
	}

	void add_test(UnitTest* test)
	{
		++total_tests;
		test->next = root;
		root = test;
	}
};


#define TEST_CATEGORY(name) UnitTestCategory _category(#name);

#define TEST_VERIFY(condition, name)\
	UnitTest name##test(#name, __LINE__, !(condition));\
	_category.add_test(&name##test)

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

#define UNITTEST(name)\
	void _unittest_##name##_execute();\
	static unittest::UnitTest _unittest_##name(#name, _unittest_##name##_execute);\
	void _unittest_##name##_execute()

#define TEST_ASSERT(condition, name)\
	if (!(condition))\
	{\
		fprintf(stdout, "'%s' FAILED (line = %i)\n", #name, __LINE__);\
		unittest::UnitTest::increment_failures();\
	}

#if 0
USAGE:

UNITTEST(name)
{
	int array[30];

	for (int i = 0; i < 30; ++i)
	{
		TEST_ASSERT(i == array[i], "array assignment");
	}

}

int main(int argc, char** argv)
{
	unittest::UnitTest::execute();
	return 0;
}

#endif

namespace unittest
{
	typedef void (*unit_test_function)();

	struct UnitTest
	{
		// per unit-test values
		const char* name;
		UnitTest* next;
		unit_test_function function;

		static UnitTest* root;
		static size_t failures;

		UnitTest(const char* test_name, unit_test_function test_function) :
			name(test_name),
			function(test_function)
		{
			// insert itself into the list
			next = UnitTest::root;
			UnitTest::root = this;
		}

		// run this test
		void run()
		{
			fprintf(stdout, "unit test '%s'...\n", name);
			function();
		}

		// iterate over and execute all unit tests
		static void execute()
		{
			UnitTest::failures = 0;

			if (UnitTest::root)
			{
				UnitTest* test = UnitTest::reverse_list(UnitTest::root);
				while(test)
				{
					test->run();
					test = test->next;
				}
			}

			fprintf(stdout, "total failures: %zu\n", UnitTest::failures);
		}

		static void increment_failures()
		{
			UnitTest::failures++;
		}

	private:
		static UnitTest* reverse_list(UnitTest* root)
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
	};

	UnitTest* UnitTest::root = nullptr;
	size_t UnitTest::failures = 0;
} // namespace unittest

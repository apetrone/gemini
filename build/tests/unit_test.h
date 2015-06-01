#include <assert.h>
#include <stdio.h>

struct UnitTest
{
	const char* name;
	int line_number;
	UnitTest* next;
	bool failed;
	
	UnitTest(const char* test_name, int line, bool failed) :
		name(test_name),
		line_number(line),
		failed(failed)
	{
	}
};


struct UnitTestCategory
{
	const char* name;
	uint32_t failed_tests;
	uint32_t total_tests;
	UnitTest* next;
	
	UnitTestCategory(const char* suite_name) :
		name(suite_name),
		failed_tests(0),
		total_tests(0),
		next(nullptr)
	{
	}
	
	~UnitTestCategory()
	{
		UnitTest* current = reverse_list();
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

		uint32_t passed_tests = (total_tests-failed_tests);
		fprintf(stdout, "* %s: %u / %u (%2.2f%%) passed\n\n", name, passed_tests, total_tests, (passed_tests/(float)total_tests)*100.0f);
	}
	
	UnitTest* reverse_list()
	{
		UnitTest* one = next;
		UnitTest* two = next->next;
		UnitTest* output = next;
		
		while(one->next)
		{
			two = one->next;
			one->next = one->next->next;
			two->next = output;
			output = two;
		}
		
		return output;
	}
	
	void add_test(UnitTest* test)
	{
		++total_tests;
		test->next = next;
		next = test;
	}
};


#define TEST_CATEGORY(name) UnitTestCategory _category(#name);

#define TEST_VERIFY(condition, name)\
	UnitTest name##test(#name, __LINE__, !(condition));\
	_category.add_test(&name##test)

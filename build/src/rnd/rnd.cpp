#include <stdio.h>
#include <stdlib.h>

#include "core/memory.h"

void test_function()
{

	fprintf(stdout, "Hello, World!\n");
}


int main(int argc, char** argv)
{
	memory::startup();
	test_function();
	memory::shutdown();
	return 0;
}
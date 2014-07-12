#include <stdio.h>
#include <stdlib.h>

#include <gemini/mem.h>
#include <gemini/core.h>
#include <gemini/core/filesystem.h>

void test_function()
{
	fprintf(stdout, "Hello, World!\n");
	

	
	size_t buffer_length = 0;
	void* buffer = core::filesystem::file_to_buffer("input.blend", 0, &buffer_length);
	
	// do something with buffer

	DEALLOC(buffer);
}


int main(int argc, char** argv)
{
	memory::startup();
	
	core::startup();

	test_function();

	core::shutdown();
	
	memory::shutdown();
	return 0;
}
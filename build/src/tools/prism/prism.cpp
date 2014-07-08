#include <stdio.h>
#include <stdlib.h>

#include <gemini/core/memory.hpp>
#include <gemini/core/filesystem.h>





void test_function()
{
	fprintf(stdout, "Hello, World!\n");
	
	gemini::core::filesystem::FileSystem* fs = gemini::core::filesystem::instance();
	
	gemini::core::filesystem::File* source_file = fs->open("input.blend");
	
	const char* buffer = source_file->create_buffer_from_file();
	
	
	source_file->destroy_buffer(buffer);
	
	fs->close(source_file);
}


int main(int argc, char** argv)
{
	gemini::core::startup();
//	memory::startup();
	test_function();
//	memory::shutdown();
	gemini::core::shutdown();
	return 0;
}
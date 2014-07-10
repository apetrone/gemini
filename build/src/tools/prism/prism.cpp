#include <stdio.h>
#include <stdlib.h>

#include <gemini/core.h>
#include <gemini/core/filesystem.h>

using namespace gemini::core;

void test_function()
{
	fprintf(stdout, "Hello, World!\n");
	
	FileSystem* fs = FileSystem::instance();
	if (fs)
	{
		gemini::core::File* source_file = fs->open("input.blend");
		const char* buffer = source_file->create_buffer_from_file();

		source_file->destroy_buffer(buffer);
		fs->close(source_file);
	}
}


int main(int argc, char** argv)
{
	gemini::memory::startup();
	
	gemini::core::startup();

	test_function();

	gemini::core::shutdown();
	
	gemini::memory::shutdown();
	return 0;
}
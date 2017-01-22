// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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
#include <core/argumentparser.h>
#include <core/core.h>
#include <core/logging.h>
#include <core/stackstring.h>
#include <core/typedefs.h>

#include <runtime/runtime.h>

enum AssetCompilerError
{
	AssetCompilerError_None = 0,
	AssetCompilerError_Generic = 1
};

#include <vector>

using namespace gemini;

int main(int argc, char** argv)
{
	// parse command line values
	std::vector<std::string> arguments;
	core::argparse::ArgumentParser parser;
	core::StackString<MAX_PATH_SIZE> content_path;

	runtime_load_arguments(arguments, parser);

	core::argparse::VariableMap vm;
	const char* docstring = R"(
Usage:
	--assets=<content_path>

Options:
	-h, --help  Show this help screen
	--version  Display the version number
	--assets=<content_path>  The path to load content from
	)";

	if (parser.parse(docstring, arguments, vm, "1.0.0-alpha"))
	{
		std::string path = vm["--assets"];
		content_path = platform::make_absolute_path(path.c_str());
	}
	else
	{
		return AssetCompilerError_Generic;
	}


	platform::Result result = core_startup();
	if (result.failed())
	{
		LOGE("Error loading core library. Exiting.\n");
		return AssetCompilerError_Generic;
	}

	LOGV("asset compiler\n");

	core_shutdown();
	return AssetCompilerError_None;
}

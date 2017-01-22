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


void asset_compiler_convert(const char* source, const char* destination)
{
	LOGV("source_asset_path = %s\n", source);
	LOGV("destination_asset_path = %s\n", destination);
}


int asset_compiler_main()
{
	platform::Result result = core_startup();
	if (result.failed())
	{
		LOGE("Error loading core library. Exiting.\n");
		return AssetCompilerError_Generic;
	}

	// parse command line values
	std::vector<std::string> arguments;
	core::argparse::ArgumentParser parser;

	runtime_load_arguments(arguments, parser);

	core::argparse::VariableMap vm;
	const char* docstring = R"(
Usage:
	<source_asset_path> <destination_asset_path>


Options:
	-h, --help  Show this help screen
	--version  Display the version number
	)";

	if (!parser.parse(docstring, arguments, vm, "1.0.0-alpha"))
	{
		return AssetCompilerError_Generic;
	}

	std::string source_asset_path = vm["source_asset_path"];
	std::string destination_asset_path = vm["destination_asset_path"];
	asset_compiler_convert(source_asset_path.c_str(), destination_asset_path.c_str());

	core_shutdown();
	return AssetCompilerError_None;
}

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();

	PLATFORM_RETURN(asset_compiler_main());
}
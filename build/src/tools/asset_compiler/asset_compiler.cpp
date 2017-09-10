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


struct AssetCompilerSettings
{
	platform::PathString source;
	platform::PathString destination;
	core::StackString<64> target_platform;
};

void asset_compiler_convert(AssetCompilerSettings* settings)
{
	LOGV("source_asset_path = %s\n", settings->source());
	LOGV("destination_asset_path = %s\n", settings->destination());

	if (!settings->target_platform.is_empty())
	{
		LOGV("target platform = %s\n", settings->target_platform());
	}
}


int asset_compiler_main()
{
	platform::Result result = core_startup();
	if (result.failed())
	{
		LOGE("Error loading core library. Exiting.\n");
		return AssetCompilerError_Generic;
	}

	int32_t show_version = 0;
	gemini::string target_platform;
	gemini::string source_asset_path;
	gemini::string destination_asset_path;

	gemini::Allocator allocator = gemini::memory_allocator_default(MEMORY_ZONE_DEFAULT);
	Array<gemini::string> arguments(allocator);

	// parse command line values
	ArgumentParser parser;
	argparse_create(allocator, &parser);

	argparse_arg(&parser, "Set the source asset path", "--source", "-s", &source_asset_path);
	argparse_arg(&parser, "Set the destination asset path", "--destination", "-d", &destination_asset_path);
	argparse_int(&parser, "Display the version number", "--version", "-v", &show_version, ArgumentParser::Optional);
	argparse_arg(&parser, "Set the target platform [windows, linux, raspberrypi]", "--platform", "-p", &target_platform, ArgumentParser::Optional);

	runtime_load_arguments(allocator, arguments);

	if (argparse_parse(&parser, arguments) != 0)
	{
		argparse_destroy(allocator, &parser);
		runtime_destroy_arguments(allocator, arguments);
		core_shutdown();
		return AssetCompilerError_Generic;
	}

	AssetCompilerSettings settings;
	settings.source = source_asset_path.c_str();
	settings.destination = destination_asset_path.c_str();
	settings.target_platform = target_platform.c_str();

	argparse_destroy(allocator, &parser);
	runtime_destroy_arguments(allocator, arguments);


	asset_compiler_convert(&settings);


	core_shutdown();
	return AssetCompilerError_None;
}

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();

	PLATFORM_RETURN(asset_compiler_main());
}
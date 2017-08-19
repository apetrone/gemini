// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include "configloader.h"

#include <runtime/filesystem.h>

#include <core/logging.h>
#include <core/mem.h>

namespace core
{
	namespace util
	{
		bool parse_json_string_with_callback(const char* buffer, size_t buffer_length, JsonLoaderCallback callback, void* context)
		{
			bool is_success = false;

			Json::Value root;
			Json::Reader reader;
			ConfigLoadStatus status = ConfigLoad_Failure;
			is_success = reader.parse(buffer, buffer+buffer_length, root);
			if (is_success)
			{
				status = callback(root, context);
			}
			else
			{
				LOGV("json parsing failed: %s\n", reader.getFormattedErrorMessages().c_str());
			}

			return is_success && (status == ConfigLoad_Success);
		} // parse_json_string_with_callback


		bool json_load_with_callback(const char* filename, JsonLoaderCallback callback, void* context, bool)
		{
			bool is_success = false;

			// load the file into a memory buffer
			gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
			Array<unsigned char> buffer(allocator);
			core::filesystem::instance()->virtual_load_file(buffer, filename);
			if (!buffer.empty())
			{
				is_success = parse_json_string_with_callback(reinterpret_cast<char*>(&buffer[0]), buffer.size(), callback, context );
			}
			else
			{
				is_success = false;
				// Do we need this at this level? callee should handle this and
				// report an error...
//				fprintf(stderr, "ERROR loading %s\n", filename);
			}

			return is_success;
		} // json_load_with_callback
	} // namespace util
} // namespace core

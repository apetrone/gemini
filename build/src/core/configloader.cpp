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

#include <platform/mem.h>
#include <core/filesystem.h>
#include <core/logging.h>

#include "configloader.h"

namespace core
{
	namespace util
	{
		bool parse_json_string_with_callback( const char * buffer, size_t buffer_length, JsonLoaderCallback callback, void * context )
		{
			bool is_success = false;
			
			Json::Value root;
			Json::Reader reader;
			ConfigLoadStatus status;
			is_success = reader.parse( buffer, buffer+buffer_length, root );
			if ( is_success )
			{
				status = callback( root, context );
			}
			else
			{
				LOGV( "json parsing failed: %s\n", reader.getFormattedErrorMessages().c_str() );
			}
			
			return is_success && (status == ConfigLoad_Success);
		} // parse_json_string_with_callback


		bool json_load_with_callback( const char * filename, JsonLoaderCallback callback, void * context, bool path_is_relative )
		{
			size_t buffer_size = 0;
			char * buffer = 0;
			bool is_success = false;

			// load the file into a memory buffer
			buffer = core::filesystem::file_to_buffer( filename, 0, &buffer_size, path_is_relative );
			if ( buffer )
			{
				is_success = parse_json_string_with_callback( buffer, buffer_size, callback, context );
				DEALLOC(buffer);
			}
			else
			{
				is_success = false;
				LOGE( "ERROR loading %s\n", filename );
			}
			
			return is_success;
		} // json_load_with_callback
	} // mamespace util
} // namespace core
// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#include "configloader.hpp"
#include "filesystem.hpp"
#include <slim/xlog.h>
#include "memory.hpp"

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
		char * buffer;
		bool is_success;
		
		// check to see if this file exists
		if ( !fs::file_exists( filename, path_is_relative ) )
		{
			LOGW( "Cannot find file: %s\n", filename );
			return 0;
		}

		// load the file into a memory buffer
		buffer = fs::file_to_buffer( filename, 0, &buffer_size, path_is_relative );
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
	
	
	
}; // mamespace util
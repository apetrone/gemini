// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

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
#pragma once

namespace fs
{
	struct FileStats
	{
		unsigned int last_modified_timestamp;
	};

	// the root directory is where the binary lives
	void root_directory( char * path, int size );
	const char * root_directory();

	// the content directory is where resources for this application can be found
	void content_directory( const char * path, int size );
	const char * content_directory();
	
	// Load a file into buffer. The pointer is returned.
	// bufferLength will contain the size of the buffer
	// if buffer is null, a new buffer is allocated and must be freed after use
	// if buffer is not null, bufferLength should contain the size of the buffer which will not be exceeded.        
	char * file_to_buffer( const char * filename, char * buffer, int * bufferLength, bool path_is_relative=true );
	
	// accepts path as a string with len: MAX_PATH_SIZE (as defined in platform.h)        
	void absolute_path_from_relative( char * fullpath, const char * relativepath, const char * content_directory = 0 );
	void relative_path_from_absolute( char * relative_path, const char * absolute_path, const char * content_directory = 0 );
	
	void truncate_string_at_path( char * path, const char * substr );

	int read_file_stats( const char * fullpath, FileStats & file_stats );
	
#if !PLATFORM_IS_MOBILE
	bool file_exists( const char * path, bool path_is_relative=true );
	bool directory_exists( const char * path, bool path_is_relative=true );
#endif
	
}; // namespace fs
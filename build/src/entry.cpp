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
#include <iostream>

#include <string.h>
#include <yajl/yajl_tree.h>

int main( int argc, char ** argv )
{
	char error_buffer[1024] = {0};
	char input[] = "{\n\"name\": \"adam\"\n}";
	yajl_val node;
	
	node = yajl_tree_parse( (const char*)input, error_buffer, 1024 );
	if ( !node )
	{
		fprintf( stderr, "parse_error: " );
		if (strlen(error_buffer))
		{
			fprintf( stderr, " %s", error_buffer );
		}
		else
		{
			fprintf( stderr, "Unknown error!" );
		}
		fprintf( stderr, "\n" );
		return 1;
	}
	
	
	const char * path[] = {"name", 0};

	yajl_val value = yajl_tree_get( node, path, yajl_t_string );
	if ( value )
	{
		printf( "%s\n", YAJL_GET_STRING(value) );
	}
	else
	{

		printf( "yep, you're crazy.\n" );
	}
	
	yajl_tree_free( node );
	
	return 0;
}
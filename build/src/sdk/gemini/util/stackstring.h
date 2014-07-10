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
#pragma once

#include <gemini/platform.h>
#include <gemini/util/asciistring.h> // for xstr_cat

template <unsigned int size, class Type=char>
struct StackString
{
	Type _data[ size ];
	unsigned int _length;
	
	StackString( int flags = 0 )
	{
		if( flags == 0 )
		{
			clear();
		}
	}
	
	StackString( const Type * s )
	{
		clear();
		*this = s;
	}
	
	unsigned int max_size() const
	{
		return size;
	}
	
	void clear()
	{
		memset( _data, 0, size );
		_length = 0;
	}
	
	unsigned int find_length()
	{
		_length = xstr_len( _data );
		return _length;
	}
	
	void operator= (const Type * data)
	{
		//printf( "data: %s\n", data );
		if ( data )
		{
			clear();
			_length = xstr_len(data);
			xstr_ncpy( _data, data, _length );
		}
	}
	
	const Type & operator[] ( int index ) const
	{
		return _data[ index ];
	}

	Type & operator[] ( int index )
	{
		return _data[ index ];
	}

	Type * find_last_slash() const
	{
		// check backslash and forward slash
		Type * pos = strrchr( (Type*)_data, '\\' ); // look for backward slash

		if ( pos )
			return pos;

		pos = strrchr( (Type*)_data, '/' ); // look for fwd slash

		return pos;
	}
	
	const Type * basename() const
	{
		Type * pos = find_last_slash();
		if ( pos )
		{
			return (pos+1);
		}

		return _data;
	}

	Type * dirname( StackString<size, Type> & out ) const
	{
		Type * pos = find_last_slash();

		if ( pos )
		{
			out = _data;
			out[ pos-_data ] = PATH_SEPARATOR;
			out[ pos-_data+1 ] = '\0';

			return pos;
		}

		return 0;
	}
	
	const Type * extension() const
	{
		Type * p = strrchr( (Type*)_data, '.' );
		if ( !p )
		{
			return _data;
		}
		return p+1;
	}
	
	void remove_extension()
	{
		const Type * p = extension();
		if ( p > 0 )
		{
			_data[(p-_data-1)] = '\0';
		}
	}
	
	void normalize( Type prefer = '/' )
	{
		for( int i = 0; i < _length; ++i )
		{
			if ( _data[i] == '\\' || _data[i] == '/' )
			{
				_data[i] = prefer;
			}
		}
	}
	
	void append( const Type * s )
	{
		if (_length + xstr_len(s) < size)
		{
			xstr_cat( _data, s );
		}
		else
		{
			assert( 0 );
		}
	}
	
	const Type *operator ()()
	{
		return _data;
	}

	
	void lshift( int pos, int count )
	{
		char * src;
		char temp[ size ];
		memset( temp, 0, size );
		memcpy( temp, _data, size );
		src = &temp[pos+count];
		
		memcpy( _data+pos, src, size-pos );
	}
	
	void shift( int pos, int count )
	{
		char * src;
		char temp[ size ];
		memset( temp, 0, size );
		memcpy( temp, _data, size );
		src = &temp[pos];
		
		for( int i = pos; i < size; ++i )
		{
			if( *src == '\0' )
			{
				break;
			}
			
			if( i < (pos+count) )
			{
				_data[ i ] = ' ';
			}
			else
			{
				_data[ i ] = *src;
				src++;
			}
		}
	}
	
	void replace( const Type * s1, const Type * s2 )
	{
		// replacement can be larger, smaller or equal size
		size_t srclen = xstr_len(s1);
		size_t dstlen = xstr_len(s2);
		
		char * p = _data;
		while( (p = strstr( p, s1 )) )
		{
			// equal size
			if ( srclen == dstlen )
			{
				for( int i = 0; i < srclen; ++i )
				{
					memcpy( p, s2, dstlen );
				}
				p++;
			}
			else if ( srclen > dstlen )
			{
				lshift( (p-_data), (srclen-dstlen) );
				memcpy( p, s2, srclen );
				p++;
			}
			else if ( srclen < dstlen )
			{
				shift( (p-_data), (dstlen-srclen) );
				memcpy( p, s2, dstlen );
				p++;
			}
		}
	}
}; // class StackString
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

#include <assert.h>
#include <platform/platform.h>
#include <core/str.h>

template <unsigned int size, class Type=char>
struct StackString
{
private:
	typedef StackString<size, Type> StackStringType;
	
public:
	Type _data[ size ];
	unsigned int _length;
	
	StackString( int flags = 0 )
	{
		if( flags == 0 )
		{
			clear();
		}
	}
	
	StackString(const Type * s)
	{
		clear();
		if (s)
		{
			copy_data(s);
		}
	}
	
	unsigned int max_size() const
	{
		return size;
	}
	
	void copy_data(const Type* source)
	{
		clear();
		_length = gemini::core::str::len(source);
		gemini::core::str::copy(_data, source, _length );
	}
	
	void clear()
	{
		memset( _data, 0, size );
		_length = 0;
	}
	
	bool is_empty() const
	{
		return _length == 0;
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
			copy_data(data);
		}
	}
	
	bool operator== (const StackStringType& other) const
	{
		assert(_length != 0 && _data != nullptr);
		if (other._length != _length)
		{
			return false;
		}
		
		const Type* data = other._data;
		const Type* self = _data;
		for (size_t i = 0; i < other._length; ++i)
		{
			if (*data != *self)
			{
				return false;
			}
			
			++data;
			++self;
		}
		
		return true;
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
	
	StackStringType basename()
	{
		// return the basename of a filepath. (just the filename)
		// this replicates the behavior in python
		Type * pos = find_last_slash();
		if (pos)
		{
			StackStringType out;
			out = (pos+1);
			return out;
		}

		return *this;
	}


	StackStringType dirname()
	{
		// return the dirname of a filepath.
		// this replicates the behavior in python
		Type * pos = find_last_slash();

		if ( pos )
		{
			StackStringType out;
			out = _data;
			out._length = (pos-_data);
			out[ out._length ] = '\0';
			return out;
		}
		
		return *this;
	}
	
	const Type* extension() const
	{
		Type * p = strrchr( (Type*)_data, '.' );
		if ( !p )
		{
			return _data;
		}
		return p+1;
	}
	
	StackStringType& remove_extension()
	{
		const Type * p = extension();
		if ( p > 0 )
		{
			size_t location = (p-_data-1);
			
			_data[location] = '\0';
			_length = gemini::core::str::len(_data);
		}
		
		return *this;
	}
	
	void normalize(Type prefer = PATH_SEPARATOR)
	{
		for(unsigned int i = 0; i < _length; ++i)
		{
			if ( _data[i] == '\\' || _data[i] == '/' )
			{
				_data[i] = prefer;
			}
		}
	}
	
	StackStringType& append(const Type * s)
	{
		if (s)
		{
			if (_length + gemini::core::str::len(s) < size)
			{
				gemini::core::str::cat(_data, s);
				_length = gemini::core::str::len(_data);
			}
			else
			{
				assert( 0 );
			}
		}
		
		return *this;
	}
	
	StackStringType& append(const StackStringType& in)
	{
		*this = append(in._data);
		
		return *this;
	}
	
	const Type *operator ()()
	{
		return _data;
	}
	
	StackStringType& strip_trailing(char character)
	{
		if (_length > 0)
		{
			if (_data[_length-1] == character)
			{
				_data[_length-1] = 0;
				_length--;
			}
		}
		
		return *this;
	}

	
	void lshift(int pos, int count)
	{
		char * src;
		char temp[ size ];
		memset( temp, 0, size );
		memcpy( temp, _data, size );
		src = &temp[pos+count];
		
		memcpy( _data+pos, src, size-pos );
	}
	
	void shift(int pos, int count)
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
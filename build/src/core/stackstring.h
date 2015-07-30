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
#pragma once

#include "util.h"
#include "str.h"

#ifndef memset
	#include <string.h>
#endif

#include <assert.h>

namespace core
{
	template <unsigned int maximum_size, class Type=char>
	struct StackString
	{
	private:
		typedef StackString<maximum_size, Type> StackStringType;
		
	public:

		Type _data[maximum_size];
		unsigned int _length;
		
		StackString()
		{
			clear();
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
			return maximum_size;
		}
		
		void copy_data(const Type* source)
		{
			clear();
			recompute_size(source);
			assert(_length < maximum_size-1);
			core::str::copy(_data, source, _length+1);
		}
		
		void clear()
		{
			memset(_data, 0, maximum_size);
			_length = 0;
		}
		
		bool is_empty() const
		{
			return _length == 0;
		}
		
		size_t size() const
		{
			return _length;
		}
		
		void recompute_size(const Type* source)
		{
			_length = core::str::len(source);
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

		Type* find_last_slash() const
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
			Type* p = strrchr( (Type*)_data, '.' );
			if ( !p )
			{
				return _data;
			}
			return p+1;
		}
		
		StackStringType& remove_extension()
		{
			const Type* p = extension();
			if (p != nullptr)
			{
				size_t location = (p-_data-1);
				
				_data[location] = '\0';
				_length = core::str::len(_data);
			}
			
			return *this;
		}
		
		void normalize(Type prefer)
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
				if (_length + core::str::len(s) < maximum_size)
				{
					core::str::cat(_data, s);
					_length = core::str::len(_data);
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
		
		const Type* operator ()() const
		{
			return _data;
		}
		
		const char* c_str() const
		{
			return _data;
		}
		
		StackStringType& strip_trailing(char character)
		{
			if (_length > 0)
			{
				while (_data[_length-1] == character)
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
			char temp[ maximum_size ];
			memset( temp, 0, maximum_size );
			memcpy( temp, _data, maximum_size );
			src = &temp[pos+count];
			
			memcpy( _data+pos, src, maximum_size-pos );
		}
		
		void shift(int pos, int count)
		{
			char * src;
			char temp[ maximum_size ];
			memset( temp, 0, maximum_size );
			memcpy( temp, _data, maximum_size );
			src = &temp[pos];
			
			for( int i = pos; i < maximum_size; ++i )
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
		
		bool startswith(const Type* s1) const
		{
			size_t string_length = core::str::len(s1);
			for(unsigned int i = 0; i < string_length; ++i)
			{
				if (_data[i] != s1[i])
				{
					return false;
				}
			}
			
			return true;
		} // startswith
		
		StackStringType substring(size_t start, size_t len = maximum_size)
		{
			StackStringType output;
			if (len > (maximum_size-start))
			{
				len = maximum_size-start;
			}
		
			memcpy(output._data, &_data[start], len);
			output._length = len;
			
			return output;
		} // substring
		
	}; // class StackString
} // namespace core

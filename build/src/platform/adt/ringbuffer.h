// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

namespace adt
{
	template <class Type, unsigned int MaxElements>
	struct RingBuffer
	{
		unsigned int head;
		unsigned int tail;
		Type data[ MaxElements ];
		
		RingBuffer()
		{
			head = tail = 0;
			memset( data, 0, sizeof(Type)*MaxElements );
		}
		
		void append( const Type & a )
		{
			memcpy( &data[ tail ], &a, sizeof(Type) );
			tail = advance( tail );
		}
		
		Type * get( unsigned int index )
		{
			if ( index > (MaxElements-1) )
			{
				return 0;
			}
			
			return &data[ index ];
		}
		
		void get_head( Type * type )
		{
			memcpy( type, &data[ head ], sizeof(Type) );
		}
		
		unsigned int size_mask() const
		{
			return (MaxElements-1);
		}
		
		void next( Type * type )
		{
			memcpy( type, &data[ head ], sizeof(Type) );
			head = advance(head);
		}
		
		void prev( Type * type )
		{
			memcpy( type, &data[ tail ], sizeof(Type) );
			tail = rewind(tail);
		}
		
		unsigned int max_size() const
		{
			return MaxElements;
		}
		
		unsigned int size()
		{
			return count();
		}
		
		unsigned int count()
		{
			if ( tail >= head )
				return (tail-head);
			
			// tail precedes head, so we must do a little math...
			//return (MaxElements - head) + tail;
			return head-tail;
		}
		
		Type * last()
		{
			return &data[ tail ];
		}
		
		void reset()
		{
			head = 0;
			tail = 0;
		}
		
		unsigned int advance( unsigned int v )
		{
			return (v+1 & size_mask());
			//return ((v+1) % MaxElements);
		}
		
		unsigned int rewind( unsigned int v )
		{
			return (v-1 & size_mask());
		}
		
		void print_status()
		{
			printf( "Head: %i, Tail: %i\n", head, tail );
		}
		
		void print_values()
		{
			printf( "-----------------------------------\n" );
			for( int i = 0; i < max_size(); ++i )
			{
				printf( "[%02i] %i", i, data[ i ] );
				if ( i == head )
					printf( " <- HEAD" );
				
				if ( i == tail )
					printf( " <- TAIL" );
				
				printf( "\n" );
			}
			printf( "-----------------------------------\n" );
		}
	};
} // namespace adt
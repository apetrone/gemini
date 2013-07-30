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

#if 0
USAGE:
	HashTable<int> table;

	table.set( "adam", 42 );

	if ( table.contains( "adam" ) )
	{
		int adam_value = table.get( "adam" );
		// ...
	}
#endif

#include "memory.hpp"
#include "xstr.h" // for string operations
#include "util.hpp"
#include <vector>
#include <slim/xlog.h>

#define HASHTABLE_SEED 42

template <class Type, unsigned int MaxTableSize=256>
class HashTable
{
private:

	struct Bucket
	{
		char * key;
		Type data;
		Bucket * next;
		
		Bucket()
		{
			key = 0;
			next = 0;
		}
		
		~Bucket()
		{
			if ( key )
			{
				DEALLOC(key);
				Bucket * bucket = next;
				Bucket * last = 0;
				
				while( bucket )
				{
					last = bucket;
					bucket = bucket->next;
					
					DESTROY(Bucket, last);
				}
			}
			next = 0;
			key = 0;
		}
		
		void set_key( const char * name )
		{
			assert( key == 0 );
			size_t name_length = xstr_len(name);
			this->key = (char*)ALLOC( name_length+1 );
			assert( this->key != 0 );
			this->key[name_length] = 0;
			xstr_ncpy(this->key, name, name_length);
		} // set_key
	}; // Bucket
		
	Type default_value;
	unsigned int table_size;
	Bucket * bucket_table;

	unsigned int hash_for_key( const char * key )
	{
		return util::hash_32bit( key, xstr_len(key), HASHTABLE_SEED) & (table_size-1);
	} // hash_for_key
	
	typename HashTable::Bucket * find_bucket( const char * key )
	{
		unsigned int hash = hash_for_key(key);
//		LOGV( "looking for '%s'; hash is %i\n", key, hash );
		Bucket * bucket = &bucket_table[ hash ];
		if ( bucket->key )
		{
			do
			{
				if ( xstr_nicmp(key, bucket->key, xstr_len(bucket->key)) == 0 )
				{
					return bucket;
				}
				
				bucket = bucket->next;
			} while( bucket );
		}
		
		return 0;
	} // find_bucket
	
public:


	HashTable( unsigned int max_table_size = MaxTableSize )
	{
		// must be a power of two
		assert( (MaxTableSize & (MaxTableSize-1)) == 0 );
		table_size = MaxTableSize;
		bucket_table = CREATE_ARRAY(Bucket, table_size);
		memset( bucket_table, 0, sizeof(Bucket)*table_size );
	}
	
	~HashTable()
	{
		purge();
	}

	bool contains( const char * key )
	{
		return (find_bucket( key ) != 0);
	} // contains
	
	void set( const char * key, const Type & value )
	{
		unsigned int hash = hash_for_key(key);
//		LOGV( "hash for '%s' is %i\n", key, hash );
		Bucket * first = &bucket_table[ hash ];
		Bucket * bucket = find_bucket( key );
		if ( bucket )
		{
//			LOGV("bucket already exists. returning\n" );
			return;
		}
		
		// the first bucket is unused; set the key
		if ( !first->key )
		{
//			LOGV( "setting first bucket to '%s' -> %i\n", key, value );
			first->set_key(key);
			first->data = value;
			return;
		}
		
		// we have to create a new bucket and insert that into a linked list
		Bucket * block = CREATE(Bucket);
		assert( block != 0 );
		
//		LOGV("creating a new bucket for '%s'\n", key);
		block->set_key(key);
		block->data = value;
		
		block->next = first->next;
		first->next = block;
	} // assign
	
	const Type & get( const char * key )
	{
		HashTable::Bucket * bucket = find_bucket( key );
		if ( bucket )
		{
			return bucket->data;
		}
		
		return default_value;
	} // get

	void purge()
	{
		DESTROY_ARRAY(Bucket, bucket_table, table_size);	
	} // purge

}; // HashTable


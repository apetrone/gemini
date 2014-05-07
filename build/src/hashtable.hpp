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
#include <slim/xstr.h> // for string operations
#include "util.hpp"
#include <vector>
#include <slim/xlog.h>

#define HASHTABLE_SEED 42

typedef unsigned int HashID;

template <class _KeyType, class _ValueType>
struct HashMapRecord
{
	_KeyType key;
	_ValueType value;
	HashMapRecord* next;
	
	HashMapRecord()
	{
		next = 0;
		memset(this, 0, sizeof(HashMapRecord<_KeyType, _ValueType>));
	}
}; // HashMapRecord

// Generic HashTablePolicy
template <class _KeyType, class _ValueType>
struct HashTablePolicy
{
	typedef HashMapRecord<_KeyType, _ValueType> RecordType;

	void init_record(HashMapRecord<_KeyType, _ValueType>& record)
	{
		record.key = UINT_MAX;
		record.value = _ValueType(0);
	}

	HashID compute_hash(const _KeyType& key)
	{
		return util::hash_32bit(&key, sizeof(_KeyType), 42);
	}
	
	void assign_key(const _KeyType& key, HashMapRecord<_KeyType, _ValueType>& record)
	{
		record.key = key;
	}
	
	void remove_key(HashMapRecord<_KeyType, _ValueType>& record)
	{
		// nothing to do
	}
	
	bool key_matches(const _KeyType& key, const HashMapRecord<_KeyType, _ValueType>& record)
	{
		return (key == record.key);
	}
	
	void assign_value(const _ValueType& value, HashMapRecord<_KeyType, _ValueType>& record)
	{
		record.value = value;
	}
	
	void remove_value(HashMapRecord<_KeyType, _ValueType>& record)
	{
		// nothing to do
	}
}; // HashTablePolicy

// String -> _ValueType policy
template <class _ValueType>
struct HashTablePolicy<char*, _ValueType>
{
	typedef HashMapRecord<char*, _ValueType> RecordType;

	void init_record(HashMapRecord<char*, _ValueType>& record)
	{
		record.key = 0;
		record.value = _ValueType(0);
	}
	
	HashID compute_hash(const char* key)
	{
		return util::hash_32bit(key, xstr_len(key), HASHTABLE_SEED);
	}
	
	void assign_key(const char* key, HashMapRecord<char*, _ValueType>& record)
	{
		size_t key_len = xstr_len(key);
		record.key = (char*)ALLOC(key_len+1);
		record.key[key_len] = 0;
		xstr_ncpy(record.key, key, key_len);
	}
	
	void remove_key(HashMapRecord<char*, _ValueType>& record)
	{
		// nothing to do
		if (record.key)
		{
			DEALLOC(record.key);
			record.key = 0;
		}
	}
	
	bool key_matches(const char* key, const HashMapRecord<char*, _ValueType>& record)
	{
		if (!key || !record.key)
		{
			return false;
		}
		
		return (xstr_nicmp(key, record.key, xstr_len(record.key)) == 0);
	}
	
	void assign_value(const _ValueType& value, HashMapRecord<char*, _ValueType>& record)
	{
		record.value = value;
	}
	
	void remove_value(HashMapRecord<char*, _ValueType>& record)
	{
		// nothing to do
	}
}; // HashTablePolicy


// Simple implementation of a closed-addressing Chained Hash Table.

template <class _KeyType, class _ValueType, unsigned int MaxTableSize=256>
class ChainedHashTable
{
	typedef HashTablePolicy<_KeyType, _ValueType> PolicyType;
	typedef typename PolicyType::RecordType Record;

	Record* records;
	PolicyType policy;
	unsigned int table_size;
	
	unsigned int compute_hash_index(const _KeyType& key)
	{
		return policy.compute_hash(key) % table_size;
	} // compute_hash
	
	Record* find_record(const _KeyType& key)
	{
		Record* record = 0;
		unsigned int index = compute_hash_index(key);
		record = &records[index];
		
		do
		{
			if (policy.key_matches(key, *record))
			{
				return record;
			}
		
			record = record->next;
		} while(record);
		
		return record;
	} // find_HashMapRecord
	
public:
	
	ChainedHashTable(unsigned int max_table_size = MaxTableSize)
	{
		table_size = max_table_size;
		records = CREATE_ARRAY(Record, table_size);
	} // ChainedHashTable
	
	~ChainedHashTable()
	{
		purge();
	} // ~ChainedHashTable
	
	bool contains(const _KeyType& key)
	{
		return (find_record(key) != 0);
	} // contains
	
	void set(const _KeyType& key, const _ValueType& value)
	{
		unsigned int index = compute_hash_index(key);
		Record* first_record = &records[index];
		Record* location = find_record(key);
		if (location)
		{
			policy.assign_key(key, *first_record);
			policy.assign_value(value, *first_record);
		}
		else
		{
			Record* new_record = CREATE(Record);
			policy.init_record(*new_record);
			policy.assign_key(key, *new_record);
			policy.assign_value(value, *new_record);

			new_record->next = first_record->next;
			first_record->next = new_record;
		}
	} // assign
	
	const _ValueType& get(const _KeyType& key, const _ValueType& default_value)
	{
		Record* record = find_record(key);
		if (record)
		{
			return record->value;
		}
		
		return default_value;
	} // get
	
	void purge()
	{
		if (records)
		{
			for (unsigned int i = 0; i < table_size; ++i)
			{
				Record* last = 0;
				Record* child = records[i].next;
				
				while(child)
				{
					policy.remove_key(*child);
					policy.remove_value(*child);
				
					last = child;
					child = child->next;
					
					DESTROY(Record, last);
				}
				
				records[i].next = 0;
			}
		
			DESTROY_ARRAY(Record, records, table_size);
			records = 0;
		}
	} // purge
}; // ChainedHashTable

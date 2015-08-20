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

#include <core/util.h>
#include <core/mem.h>

template <class K, class T, class H = typename core::util::hash<K>, class Allocator = core::memory::SystemAllocatorType >
class HashSet
{
private:
	typedef H hash_type;
	typedef Allocator allocator_type;
	
	const float MAX_LOAD_FACTOR = 0.7f;
	typedef uint32_t HashType;
	const HashType REMOVED_SLOT = UINT32_MAX;

	struct Bucket
	{
		HashType hash;
		T value;
		K key;
		
		Bucket() : hash(0)
		{
		}
	};
	
	Bucket* table;
	uint32_t table_size;
	uint32_t used_items;
	uint32_t growth_factor;
	allocator_type& allocator;

	int32_t find_bucket(HashType hash, int32_t& bucket_index, bool inserting = false) const
	{
		// use linear probing to find the bucket
		int32_t current_index = (hash % table_size);
		for( ; ; )
		{
			bucket_index = (current_index++ % table_size);
			if (table[bucket_index].hash == 0)
			{
				return -1;
			}

			if ((table[bucket_index].hash == hash) ||
				(inserting && (table[bucket_index].hash == REMOVED_SLOT))) // this bucket has been removed, so we can use it
			{
				return bucket_index;
			}
		}
		
		// should never reach here!
		assert(0);
		return -1;
	}
	
	int32_t find_first_occupied(int32_t starting_position) const
	{
		int32_t current_index = starting_position;
		int32_t max_iterations = table_size-1;
		while(max_iterations > -1)
		{
			int32_t bucket_index = current_index % table_size;
			Bucket* bucket = &table[bucket_index];
			if (bucket->hash != 0 && bucket->hash != REMOVED_SLOT)
			{
				return bucket_index;
			}
			
			++current_index;
			--max_iterations;
		}
		
		return -1;
	}
	
	inline HashType get_hash(const K& key) const
	{
		return hash_type()(key);
	}
	
	void repopulate(size_t new_size)
	{
		Bucket* old_table = table;

		size_t total_items = table_size;
		table_size = new_size;
		table = allocate(table_size);

		// reset used item count as we need to re-insert them.
		used_items = 0;

		// if these are NOT re-inserted into the new hash; then
		// they won't be resolved correctly.
		for (size_t i = 0; i < total_items; ++i)
		{
			if (old_table[i].hash != 0)
				insert(value_type(old_table[i].key, old_table[i].value));
		}

		// free the old table
		deallocate(old_table);
	}
	
	Bucket* find_or_create_bucket(const K& key)
	{
		// check for repopulate first as this simplifies insertion
		// when the table exceeds the load factor
		if ((used_items/(float)table_size) > MAX_LOAD_FACTOR)
		{
			// if we hit this point; we need to resize the table
			repopulate(table_size*growth_factor);
		}
	
		HashType hash = get_hash(key);
		int32_t bucket_index;
		int32_t index = find_bucket(hash, bucket_index);
		Bucket* bucket = 0;
		
		if (index != -1)
		{
			return &table[index];
		}
		else
		{
			if (bucket_index != -1)
			{
				used_items++;
				bucket = &table[bucket_index];
				bucket->key = key;
				bucket->hash = hash;
			}
			else
			{
				// not sure what happened
				assert(0);
			}
		}

		
		if (bucket)
		{
			return bucket;
		}

		return find_or_create_bucket(key);
	}
	
	Bucket* allocate(uint32_t elements)
	{
		return MEMORY_NEW_ARRAY(Bucket, elements, allocator);
	}
	
	
	void deallocate(Bucket* pointer)
	{
		MEMORY_DELETE_ARRAY(pointer, allocator);
	}

public:
	typedef std::pair<K, T> value_type;


	HashSet(size_t initial_size = 16, uint32_t growth_factor = 2, Allocator& allocator_instance = core::memory::system_allocator()) :
		table(nullptr),
		table_size(initial_size),
		used_items(0),
		growth_factor(growth_factor),
		allocator(allocator_instance)
	{
		table = allocate(table_size);
	}
	
	~HashSet()
	{
		deallocate(table);
	}
	
	
	bool has_key(const K& key) const
	{
		HashType hash = get_hash(key);
		int32_t bucket_index;
		return (find_bucket(hash, bucket_index, true) != -1);
	}
	
	void insert(const value_type& vt)
	{
		Bucket* bucket = find_or_create_bucket(vt.first);
		bucket->value = vt.second;
	}
	
	void remove(const K& key)
	{
		HashType hash = get_hash(key);
		int32_t index = find_bucket(hash);
		if (index != -1)
		{
			table[index].hash = REMOVED_SLOT;
			--used_items;
		}
	}
	
	void clear()
	{
		// no need to deallocate the memory, but at least reset the hashes
		used_items = 0;
		for (size_t index = 0; index < table_size; ++index)
		{
			table[index].hash = 0;
		}
	}
	
	T& get(const K& key)
	{
		Bucket* bucket = find_or_create_bucket(key);
		return bucket->value;
	}

	// get value or insert if not found
	T& operator[](const K& key)
	{
		return get(key);
	}
	
	const T& operator[](const K& key) const
	{
		HashType hash = get_hash(key);
		int32_t bucket_index;
		int32_t index = find_bucket(hash, bucket_index, true);
		assert(index != -1);
		Bucket* bucket = &table[index];
		return bucket->value;
	}

	size_t size() const
	{
		return used_items;
	}
	
	size_t capacity() const
	{
		return table_size;
	}
	
	bool empty() const
	{
		return (used_items == 0);
	}

	class Iterator
	{
		typedef HashSet<K, T, H, Allocator> container_type;

	private:
		typename container_type::Bucket* table;
		size_t index;
		size_t table_size;
		
	public:
		Iterator(typename container_type::Bucket* table, size_t index, size_t table_size) :
			table(table),
			index(index),
			table_size(table_size)
		{
		}
		
		Iterator& operator=(const Iterator& other)
		{
			this->index = other.index;
			return *this;
		}
		
		bool operator==(const Iterator& other) const
		{
			return (index == other.index);
		}
		
		bool operator!=(const Iterator& other) const
		{
			return !(*this == other);
		}
		
		const Iterator& operator++()
		{
			while(index < table_size)
			{
				container_type::Bucket* bucket = &table[++index];
				if (bucket->hash != 0)
					return *this;
			}
			return *this;
		}
		
		T& operator*()
		{
			return table[index].value;
		}

		const K& key() const
		{
			return table[index].key;
		}
		
		const T& value() const
		{
			return table[index].value;
		}
	}; // class Iterator
	
	Iterator begin() const
	{
		// if the hash set is empty; we have no valid iterator.
		int32_t index = find_first_occupied(0);
		if (index == -1)
		{
			index = table_size;
		}
		return Iterator(table, index, table_size);
	}
	
	Iterator end() const
	{
		return Iterator(table, table_size, table_size);
	}
}; // HashSet
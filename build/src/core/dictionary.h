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


#include <core/color.h>
#include <core/util.h>

namespace core
{

	enum DataType
	{
		DATA_INVALID = 0,
		DATA_INT,
		DATA_FLOAT,
		DATA_STRING,
		DATA_VECTOR3,
		
		DATA_POINTER,

		DATA_MAX
	};





	class PolicyBase
	{
	public:
		virtual ~PolicyBase() {}
		virtual DataType type() = 0;
		virtual void destroy() = 0;
		virtual void create( const void * data ) = 0;
		virtual void update( const void * type ) = 0;
		virtual void get( void * target ) = 0;

	};

	typedef PolicyBase * (*policy_creator)();

	template <class Type>
	struct keyvalues_typemap
	{
		static DataType get_type()
		{
			return DATA_INVALID;
		}
	};

	#define DECLARE_POLICY_TYPE( type, data_type )\
		template <>\
		struct keyvalues_typemap<type>\
		{\
			static DataType get_type()\
			{\
				return data_type;\
			}\
		}

	#define IMPLEMENT_POLICY( classname, name )\
		PolicyBase * name##_create()\
		{\
			return CREATE(classname);\
		}

	#define MAKE_POLICY( name )\
		name##_create



	DECLARE_POLICY_TYPE(int, DATA_INT);
	DECLARE_POLICY_TYPE(float, DATA_FLOAT);
	DECLARE_POLICY_TYPE(const char *, DATA_STRING);
	DECLARE_POLICY_TYPE(glm::vec3, DATA_VECTOR3);

	struct KeyValues
	{
		char * name;
		
		KeyValues * next;
		KeyValues * prev;
		KeyValues * parent;
		KeyValues * children;

		KeyValues();
		~KeyValues();
		
		PolicyBase * policy;
		
		static policy_creator policy_for_type( DataType type );

		template <class Type>
		void set( const char * key, const Type & value )
		{
			DataType target_type = keyvalues_typemap<Type>::get_type();
			if ( this->policy && this->policy->type() != target_type )
			{
				DESTROY(PolicyBase, this->policy);
			}
			
			policy = KeyValues::policy_for_type(target_type)();
			policy->create( &value );
		}

		template <class Type>
		Type get( const char * key, const Type & default_value )
		{
			DataType target_type = keyvalues_typemap<Type>::get_type();
			Type value = default_value;
			if ( this->policy && this->policy->type() == target_type )
			{
				policy->get( &value );
			}
			
			return value;
		}
	}; // KeyValues
	
	template <class Type, int HashTableSize=16>
	class Dictionary
	{
	private:
		typedef const char* KeyType;
		typedef uint32_t HashType;
	
		struct Bucket
		{
			char* key;
			Type data;
			Bucket* next;
			
			
			static Bucket* create()
			{
				return CREATE(Bucket);
			}
			
			static void destroy(Bucket* bucket)
			{
				DESTROY(Bucket, bucket);
			}
			
			
			Bucket()
			{
				key = 0;
				next = 0;
			}

			~Bucket()
			{
				if (key)
				{
					platform::memory::allocator().deallocate(key);
					key = 0;
				}
				
				Bucket* iter = next;
				while(iter)
				{
					Bucket* temp = iter;
					Bucket::destroy(temp);
					iter = iter->next;
				}
			}
			
			Bucket(const Bucket& other)
			{
				*this = other;
			}
			
			const Bucket& operator=(const Bucket& other)
			{
				key = other.key;
				data = other.data;
				next = other.next;
				return *this;
			}
			
			bool key_matches(KeyType key) const
			{
				return (core::str::case_insensitive_compare(this->key, key, 0) == 0);
			}
			
			void set_key(KeyType newkey)
			{
				if (key != 0)
				{
					platform::memory::allocator().deallocate(key);
				}
				
				size_t key_length = core::str::len(newkey);
				this->key = (char*)platform::memory::allocator().allocate(key_length+1, __FILE__, __LINE__);
				this->key[key_length] = 0;
				core::str::copy(this->key, newkey, key_length);
			}
			
			void set_value(const Type& value)
			{
				data = value;
			}
			
			bool is_used() const
			{
				return (key != 0);
			}
		};


		Bucket* table[HashTableSize];

	private:
		HashType hash(KeyType key) const
		{
			return core::util::hash_32bit(key, core::str::len(key), 0);
		} // hash
		
		uint32_t get_table_index(KeyType key) const
		{
			return (hash(key) % HashTableSize);
		} // get_table_index
		
		const Bucket* find_bucket(uint32_t index, KeyType key) const
		{
			const Bucket* bucket = table[index];
			if (bucket && bucket->is_used())
			{
				while(bucket)
				{
					if (bucket->key_matches(key))
					{
						return bucket;
					}
					bucket = bucket->next;
				}
			}
			
			return 0;
		} // find_bucket


	public:
		Dictionary()
		{
			for (size_t index = 0; index < HashTableSize; ++index)
			{
				table[index] = 0;
			}
		}
		
		~Dictionary()
		{
			clear();
		}
		
		void clear()
		{
			for (size_t index = 0; index < HashTableSize; ++index)
			{
				if (table[index])
				{
					Bucket::destroy(table[index]);
					table[index] = 0;
				}
			}
		}
		
		void insert(KeyType key, const Type& value)
		{
			HashType index = get_table_index(key);
			const Bucket* existing_node = find_bucket(index, key);
			if (!existing_node)
			{
				Bucket* bucket = table[index];
				Bucket* newbucket = 0;
				
				newbucket = Bucket::create();
				newbucket->set_key(key);
				newbucket->set_value(value);
				
				// bucket is not used
				if (!bucket)
				{
					table[index] = newbucket;
				}
				else
				{
					// need to insert a new bucket in the chain
					newbucket->next = bucket;
					table[index] = newbucket;
				}
			}
			else
			{
				// update the data
				Bucket* bucket = table[index];
				bucket->set_value(value);
			}
			
		} // insert
		
		bool has_key(KeyType key) const
		{
			return (find_bucket(get_table_index(key), key) != 0);
		} // has_key
		
		bool get(KeyType key, Type& value)
		{
			const Bucket* bucket = find_bucket(get_table_index(key), key);
			if (bucket)
			{
				value = bucket->data;
				return true;
			}
			
			return false;
		} // get

	}; // Dictionary
	
	template <class K, class T>
	class HashSet
	{
	private:
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
		
		
		int32_t find_bucket(int32_t hash, int32_t& bucket_index, bool inserting = false) const
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
			while(max_iterations > 0)
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
			return core::util::hash32(key);
		}
		
		void repopulate(size_t new_size)
		{
			Bucket* old_table = table;
			

			size_t total_items = table_size;							
			table_size = new_size;
			table = allocate(table_size);

			
			// straight up copying is much faster than re-inserting
			for (size_t i = 0; i < total_items; ++i)
			{
				table[i] = old_table[i];
			}
			
			// free the old table
			deallocate(old_table, total_items);
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
			return CREATE_ARRAY(Bucket, elements);
		}
		
		
		void deallocate(Bucket* pointer, size_t elements)
		{
			DESTROY_ARRAY(Bucket, pointer, elements);
		}

	public:
		typedef std::pair<K, T> value_type;
	
	
		HashSet(size_t initial_size = 16, uint32_t growth_factor = 2) :
			table(nullptr),
			table_size(initial_size),
			used_items(0),
			growth_factor(growth_factor)
		{
			table = allocate(table_size);
		}
		
		~HashSet()
		{
			deallocate(table, table_size);
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
			int32_t bucket_index = (hash % table_size);
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
		private:
			HashSet<K, T>::Bucket* table;
			size_t index;
			size_t table_size;
			
		public:
			Iterator(HashSet<K, T>::Bucket* table, size_t index, size_t table_size) :
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
					HashSet<K, T>::Bucket* bucket = &table[++index];
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
			return Iterator(table, find_first_occupied(0), table_size);
		}
		
		Iterator end() const
		{
			return Iterator(table, table_size, table_size);
		}
	}; // class OpenAddressingHash
	
	
	
	
	
} // namespace core
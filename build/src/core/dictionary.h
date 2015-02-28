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
} // namespace core
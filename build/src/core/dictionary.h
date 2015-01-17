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

namespace gemini
{
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
		
		
		class Dictionary
		{
		public:
			virtual ~Dictionary() {}
		};
	} // namespace core
} // namespace gemini

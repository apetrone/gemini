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

#include "dictionary.h"

#include <core/typedefs.h>
#include <core/logging.h>
#include <core/str.h>
#include <core/mathlib.h>

namespace core
{
	template <class Type>
	class PODPolicy : public PolicyBase
	{
		Type value;
		
	public:
		virtual DataType type() { return keyvalues_typemap<Type>::get_type(); }
		
		virtual void destroy()
		{
			
		}
		
		virtual void create( const void * data )
		{
			update( data );
		}
		
		virtual void update( const void * type )
		{
			value = *((Type*)type);
		}
		
		virtual void get( void * target )
		{
			Type * p = (Type*)target;
			*p = value;
		}
	};


	IMPLEMENT_POLICY(PODPolicy<int>, IntPolicy);
	IMPLEMENT_POLICY(PODPolicy<float>, FloatPolicy);





	class Vec3Policy : public PolicyBase
	{
		glm::vec3 * vector;
		
	public:
		virtual DataType type() { return keyvalues_typemap<glm::vec3>::get_type(); }
		
		virtual void destroy()
		{
			using glm::vec3;
			MEMORY_DELETE(vector, platform::memory::global_allocator());
		}
		
		virtual void create( const void * data )
		{
			MEMORY_NEW(glm::vec3, platform::memory::global_allocator());
			
			update( data );
		}
		
		virtual void update( const void * type )
		{
			*vector = *((glm::vec3*)type);	
		}
		
		virtual void get( void * target )
		{
			glm::vec3 * p = (glm::vec3*)target;
			*p = *vector;
		}
	};
	IMPLEMENT_POLICY(Vec3Policy, Vec3Policy);



	policy_creator KeyValues::policy_for_type( DataType type )
	{
		LOGV( "policy type: %i\n", type );
		
		assert( type >= 0 && type < DATA_MAX );

		policy_creator policy_table[DATA_MAX] =
		{
			0,
			MAKE_POLICY(IntPolicy),
			MAKE_POLICY(FloatPolicy),
			0,
			MAKE_POLICY(Vec3Policy),
			0,
			
		};
		
		return policy_table[ type ];
	}

	KeyValues::KeyValues()
	{
		this->policy = 0;
	}

	KeyValues::~KeyValues()
	{
		if ( this->policy )
		{
			this->policy->destroy();
			
			MEMORY_DELETE(this->policy, platform::memory::global_allocator());
			this->policy = 0;
		}
	}
} // namespace core

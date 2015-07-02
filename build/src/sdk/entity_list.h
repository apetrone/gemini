// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <core/mem.h>
#include <core/typedefs.h>

#include "entity.h"

#include <list>

template <class Type>
struct EntityList
{
	// There's definitely an issue where the vector isn't deallocating
	// using the custom allocator. This may be my mis-understanding of the usage
	// pattern though. For now, switching this over to non GeminiAllocator.
	//	typedef std::vector< Type*, GeminiAllocator<Type*> > EntityVectorType;

	// Switching this over from vector to a list because we need to iterate over
	// the collection and delete items. Doing so over a vector is bad because it
	// invalidates the next iterator when an item is removed.
	//typedef std::vector< Type* > Vector;

	typedef std::list< Type* > Collection;

	Collection objects;
	
	void add( Type * object )
	{
		this->objects.push_back( object );
	} // add
	
	virtual void remove( Type * object )
	{
		for (typename Collection::iterator it = this->objects.begin(); it != this->objects.end(); ++it)
		{
			Type * obj = (*it);
			
			if ( obj == object )
			{
				//				LOGV( "removing from entity list\n" );
				objects.erase( it );
				break;
			}
		}
	} // remove
	
	void clear()
	{
		objects.clear();
	} // clear
	
	void purge()
	{
		// create a local copy that won't be modified as we traverse it.
		Collection objects = this->objects;
	
		for (typename Collection::iterator it = objects.begin(); it != objects.end(); ++it)
		{
			Entity * obj = (*it);
			delete obj;
		}
		
		clear();
	} // purge

	Type* find_with_name(const EntityName& name)
	{
		for (typename Collection::iterator it = this->objects.begin(); it != this->objects.end(); ++it)
		{
			Entity * obj = (*it);

			if (name == obj->name)
			{
				return obj;
			}
		}
		
		return 0;
	} // find_with_name
	
	
	Type * object_at_index( size_t index )
	{
		assert(index <= this->count());
		
		return objects[ index ];
	} // object_at_index
	
	size_t count() const
	{
		return objects.size();
	} // count
}; // EntityList


typedef EntityList<Entity> EntityListType;
EntityListType& entity_list();


// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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

#include <platform/typedefs.h>
#include <platform/mem.h>

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
	
	
	
	Type * find_with_name( const String & name )
	{
		for (typename Collection::iterator it = this->objects.begin(); it != this->objects.end(); ++it)
		{
			Entity * obj = (*it);
			
			if ( name == obj->name )
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

scenegraph::Node* get_entity_root();
void set_entity_root(scenegraph::Node* root);



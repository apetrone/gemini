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
#include "typedefs.h"
#include "componentlibrary.h"



ComponentLibrary::ComponentLibrary()
{
	containers.resize( MaxComponentTypes );
}

void ComponentLibrary::register_container( GenericComponentContainer * container, ComponentType type )
{
	containers[ type ] = container;
} // register_container

IComponent * ComponentLibrary::find_id( ComponentType type, unsigned int reference_id )
{
	IComponent * component = 0;
	GenericComponentContainer * gcc = container_from_type( type );
	if ( gcc )
	{
		component = gcc->find_id(reference_id);
	}
	
	return component;
}

void ComponentLibrary::step( float delta_seconds )
{
	GenericContainerVector::iterator it = containers.begin();
	for( ; it != containers.end(); ++it )
	{
		(*it)->step( delta_seconds );
	}
} // step

void ComponentLibrary::tick( float delta_seconds, float step_alpha )
{
	GenericContainerVector::iterator it = containers.begin();
	for( ; it != containers.end(); ++it )
	{
		(*it)->tick( delta_seconds, step_alpha );
	}
} // tick

void ComponentLibrary::purge()
{
	GenericContainerVector::iterator it = containers.begin();
	for( ; it != containers.end(); ++it )
	{
		(*it)->purge();
	}
} // purge

GenericComponentContainer * ComponentLibrary::container_from_type( ComponentType type )
{
	GenericComponentContainer * gcc = 0;
	
	assert( type >= 0 && type < containers.size() );
	gcc = containers[ type ];
	
	return gcc;
} // container_from_type
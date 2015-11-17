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
#include <gemini/typedefs.h>
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

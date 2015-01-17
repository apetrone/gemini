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
#include <core/typedefs.h>
#include "menu.h"
#include <core/logging.h>

//
// MenuItem
MenuItem::MenuItem()
{
	parent = 0;
	name = "empty";
	userdata = 0;
}

MenuItem::~MenuItem()
{
	purge();
}

MenuItem * MenuItem::child_at_index( unsigned int index )
{
	assert( index < children.size() );
	return children[ index ];
} // child_at_index

void MenuItem::purge()
{
	MenuItemVector::iterator it, end;
	it = children.begin();
	end = children.end();
	for( ; it != end; ++it )
	{
		MenuItem * option = (*it);
		DESTROY(MenuItem, option);
	}
	children.clear();
} // purge

MenuItem * MenuItem::add_child( const char * name )
{
	MenuItem * option = CREATE(MenuItem);
	option->name = name;
	option->parent = this;
	
	children.push_back( option );
	
	return option;
} // add_child




//
// MenuNavigator
MenuNavigator::MenuNavigator()
{
	current = &root;
}

MenuNavigator::~MenuNavigator()
{
	current = 0;
}

MenuItem * MenuNavigator::root_menu()
{
	return &root;
}

MenuItem * MenuNavigator::current_menu()
{
	return current;
}

void MenuNavigator::clear_items()
{
	root.purge();
}

void MenuNavigator::navigate_back()
{
	if ( current && current->parent )
	{
		LOGV( "Navigating to %s\n", current->parent->name );
		current = current->parent;
	}
}

void MenuNavigator::navigate_to_child( unsigned int index )
{
	current = current->child_at_index( index );
	LOGV( "New child is %s\n", current->name );
}

unsigned int MenuNavigator::child_count()
{
	if ( current )
	{
		return current->children.size();
	}
	return 0;
} // child_count

MenuItem * MenuNavigator::child_at_index( unsigned int i )
{
	return current->child_at_index( i );
}

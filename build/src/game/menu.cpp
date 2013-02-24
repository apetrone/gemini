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
#include "menu.hpp"
#include "log.h"

//
// MenuItem
MenuItem::MenuItem()
{
	parent = 0;
	name = "empty";
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

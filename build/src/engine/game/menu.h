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
#pragma once
#include <platform/mem.h>
#include <vector>

struct MenuItem;
typedef std::vector<MenuItem*, GeminiAllocator<MenuItem*> > MenuItemVector;

struct MenuItem
{
	MenuItemVector children;
	const char * name;
	MenuItem * parent;
	void * userdata;
	
	MenuItem();
	~MenuItem();
	MenuItem * child_at_index( unsigned int index );
	void purge();
	MenuItem * add_child( const char * name );
}; // MenuItem

typedef void (*foreach_menu_callback)( MenuItem * item );

class MenuNavigator
{
	MenuItem root;
	MenuItem * current;
	
public:
	MenuNavigator();
	~MenuNavigator();

	MenuItem * root_menu();
	MenuItem * current_menu();
	void clear_items();
	
	// navigate back to parent
	void navigate_back();
	void navigate_to_child( unsigned int index );
	
	// child iteration
	unsigned int child_count();
	MenuItem * child_at_index( unsigned int i );
};

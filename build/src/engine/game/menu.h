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
#include <core/mem.h>
#include <vector>

struct MenuItem;
typedef std::vector<MenuItem*> MenuItemVector;

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

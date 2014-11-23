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
#include <gemini/typedefs.h>
#include <slim/xlog.h>
#include "screencontrol.h"

ScreenController::ScreenController()
{
} // ScreenController

ScreenController::~ScreenController()
{
	ScreenVector::iterator it = screens.begin();
	for( ; it != screens.end(); ++it )
	{
		IScreen * screen = (*it);
		DESTROY(IScreen, screen);
	}
} // ~ScreenController

void ScreenController::add_screen( IScreen * screen )
{
	screens.push_back( screen );
} // add_screen

IScreen * ScreenController::find_screen( const char * name ) const
{
	ScreenVector::const_iterator it = screens.begin();
	IScreen * screen;
	for( ; it != screens.end(); ++it )
	{
		screen = (*it);
		if ( xstr_nicmp(name, screen->name(), 0) == 0 )
		{
			return screen;
		}
	}
	return 0;
} // find_screen

size_t ScreenController::count_screens() const
{
	size_t total_screens = 0;
	ScreenVector::const_iterator it = screens.begin();
	for( ; it != screens.end(); ++it )
	{
		++total_screens;
	}
	
	return total_screens;
} // count_screens

IScreen * ScreenController::push_screen( const char * name, kernel::IApplication * app )
{
	IScreen * current_screen = this->active_screen();
	
	// if the new screen wasn't found; report a warning
	IScreen * screen = this->find_screen( name );
	if ( !screen )
	{
		LOGW( "Unable to find screen named '%s'\n", name );
		return current_screen;
	}
	
	// new screen is valid and doesn't match current screen
	if ( screen && screen != current_screen )
	{
		if ( current_screen )
		{
			current_screen->on_hide( app );
		}
		this->screen_stack.push( screen );
		screen->on_show( app );
	}
	
	return screen;
} // push_screen

void ScreenController::pop_screen( kernel::IApplication * app )
{
	this->screen_stack.pop();
} // pop_screen

IScreen * ScreenController::active_screen()
{
	// if the stack isn't empty, get the first item
	if ( !this->screen_stack.empty() )
	{
		return this->screen_stack.top();
	}
	
	return 0;
} // active_screen

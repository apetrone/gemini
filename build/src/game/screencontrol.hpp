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
#include "memory.hpp"

#include <vector>
#include <stack>

struct IScreen
{
	virtual ~IScreen() {}
	virtual void on_show() = 0;
	virtual void on_hide() = 0;
	virtual void on_draw() = 0;
	virtual void on_update() = 0;
	virtual const char * name() const = 0;
}; // IScreen


struct ScreenController
{
	typedef std::stack<IScreen*> ScreenStack;
	typedef std::vector<IScreen*> ScreenVector;
	ScreenVector screens;
	ScreenStack screen_stack;
	
	ScreenController();
	~ScreenController();
	
	void add_screen( IScreen * screen );
	IScreen * find_screen( const char * name ) const;
	size_t count_screens() const;
	
	// set the new active screen with name;
	IScreen * push_screen( const char * name );
	
	// pop the screen on top of the stack
	void pop_screen();
	
	IScreen * active_screen();
}; // ScreenController
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
#include "memory.h"

#include <vector>
#include <stack>

#include "kernel.h"

struct IScreen
{
	virtual ~IScreen() {}
	virtual void on_show( kernel::IApplication * app ) = 0;
	virtual void on_hide( kernel::IApplication * app ) = 0;
	virtual void on_draw( kernel::IApplication * app ) = 0;
	virtual void on_update( kernel::IApplication * app ) = 0;
	virtual void on_step( kernel::IApplication * app ) = 0;
	virtual const char * name() const = 0;
		
	virtual void on_event( kernel::KeyboardEvent & event, kernel::IApplication * app ) = 0;
	virtual void on_event( kernel::MouseEvent & event, kernel::IApplication * app ) = 0;
	virtual void on_event( kernel::TouchEvent & event, kernel::IApplication * app ) = 0;
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
	IScreen * push_screen( const char * name, kernel::IApplication * app );
	
	// pop the screen on top of the stack
	void pop_screen( kernel::IApplication * app );
	
	IScreen * active_screen();
}; // ScreenController
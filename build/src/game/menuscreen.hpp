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
#include "screencontrol.hpp"
#include "menu.hpp"
#include "assets.hpp"

#include "componentmanager.hpp" // for RenderControl

struct MenuScreen : public virtual IScreen
{
	font::Handle menu_font, title_font;
	MenuNavigator menunav;
	int current_menu;
	MenuScreen();
	
	assets::Material * mainmenu;
	renderer::VertexStream vs;
	
	RenderControl rc;
	
	virtual void on_show( kernel::IApplication * app );
	virtual void on_hide( kernel::IApplication * app );
	virtual void on_draw( kernel::IApplication * app );
	virtual void on_update( kernel::IApplication * app );
	virtual void on_step( kernel::IApplication * app );
	virtual const char * name() const;

	
	// any event that happens during the logo screen triggers a skip to the next screen
	virtual void on_event( kernel::KeyboardEvent & event, kernel::IApplication * app );
	virtual void on_event( kernel::MouseEvent & event, kernel::IApplication * app );
	virtual void on_event( kernel::TouchEvent & event, kernel::IApplication * app );
	
	
	void skip_screen( kernel::IApplication * app );
}; // MenuScreen
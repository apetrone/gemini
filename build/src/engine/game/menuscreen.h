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
#include "screencontrol.h"
#include "menu.h"
#include "assets.h"

#include "componentmanager.h" // for RenderControl

struct MenuScreen : public IScreen
{
	assets::Font * menu_font;
	assets::Font * title_font;
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

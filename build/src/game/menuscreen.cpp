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
#include "log.h"
#include "font.hpp"
#include "menuscreen.hpp"
#include "engine.hpp"
#include "input.hpp"

MenuScreen::MenuScreen()
{
	font = font::load_font_from_file( "fonts/nokiafc22.ttf", 64 );
	
	MenuItem * root = menunav.root_menu();
	MenuItem * item = 0;
	
	
	root->add_child("Play");
	root->add_child("Options");
	root->add_child("Quit");
	
	current_menu = 0;
}

void MenuScreen::on_show( kernel::IApplication * app )
{
	LOGV( "MenuScreen on show\n" );
}

void MenuScreen::on_hide( kernel::IApplication * app )
{
	LOGV( "MenuScreen on hide\n" );
}

void MenuScreen::on_draw( kernel::IApplication * app )
{
	MenuItem * current = menunav.current_menu();
	if ( current )
	{
		int height = 220;
		for( unsigned int i = 0; i < menunav.child_count(); ++i )
		{
			MenuItem * child = menunav.child_at_index(i);
			if ( child )
			{
				const char * text = child->name;
				
				Color menu_color = Color(255,255,255);
				
				if ( current_menu == i )
				{
					menu_color = Color(255,0,0);
				}

				int center = (kernel::instance()->parameters().render_width / 2);
				int font_width = font::measure_width( font, text );
				font::draw_string( font, center-(font_width/2), height, text, menu_color );
				height += 96;

			}
		}
	}
}

void MenuScreen::on_update( kernel::IApplication * app ) {}

void MenuScreen::on_step( kernel::IApplication * app ) {}

	
const char * MenuScreen::name() const
{
	return "MenuScreen";
}

void MenuScreen::skip_screen( kernel::IApplication * app )
{
	engine::engine()->screen_controller()->pop_screen( app );
}

// any event that happens during the logo screen triggers a skip to the next screen
void MenuScreen::on_event( kernel::KeyboardEvent & event, kernel::IApplication * app )
{
	bool is_up = (event.key == input::KEY_UP);
	bool is_down = (event.key == input::KEY_DOWN);
	bool is_arrow = is_up || is_down || (event.key == input::KEY_LEFT) || (event.key == input::KEY_RIGHT);
	if ( event.is_down && !is_arrow )
	{
		skip_screen( app );
	}
	else if ( event.is_down )
	{
		unsigned int total_children = menunav.child_count();
		if ( is_up )
		{
			current_menu -= 1;
			if (current_menu < 0)
			{
				current_menu = total_children-1;
			}
		}
		else if ( is_down )
		{
			current_menu += 1;
			if (current_menu > total_children-1)
			{
				current_menu = 0;
			}
		}
		
	}
}

void MenuScreen::on_event( kernel::MouseEvent & event, kernel::IApplication * app )
{
	// except mouse moved events.
	if ( event.subtype != kernel::MouseMoved )
	{
		skip_screen( app );
	}
}

void MenuScreen::on_event( kernel::TouchEvent & event, kernel::IApplication * app )
{
	skip_screen( app );
}
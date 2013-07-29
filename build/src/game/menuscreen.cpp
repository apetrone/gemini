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
#include "stackstring.hpp"

const int MENU_PLAY = 1;
const int MENU_QUIT = 2;

const int VERTICAL_START = 220;
const int VERTICAL_SPACING = 48;

MenuScreen::MenuScreen()
{
	font = font::load_font_from_file( "fonts/nokiafc22.ttf", 24 );
	
	MenuItem * root = menunav.root_menu();
	MenuItem * item;
	item = root->add_child("Play");
	item->userdata = (void*)&MENU_PLAY;
	
	root->add_child("Options");

	item = root->add_child("Quit");
	item->userdata = (void*)&MENU_QUIT;
	
	current_menu = 0;
	
	protagonist = assets::textures()->load_from_path("textures/protagonist");
	antagonist = assets::textures()->load_from_path("textures/antagonist");
	
	
	vs.reset();
	vs.desc.add( renderer::VD_FLOAT3 );
	vs.desc.add( renderer::VD_UNSIGNED_BYTE4 );
	vs.desc.add( renderer::VD_FLOAT2 );
	vs.create( 4, 6, renderer::DRAW_INDEXED_TRIANGLES );
}

void MenuScreen::on_show( kernel::IApplication * app )
{
}

void MenuScreen::on_hide( kernel::IApplication * app )
{
}

void MenuScreen::on_draw( kernel::IApplication * app )
{

	

	MenuItem * current = menunav.current_menu();
	if ( current )
	{
		int height = VERTICAL_START;
		for( unsigned int i = 0; i < menunav.child_count(); ++i )
		{
			MenuItem * child = menunav.child_at_index(i);
			if ( child )
			{
				StackString<128> text;
				Color menu_color = Color(255,255,255);
				
				if ( current_menu == i )
				{
					menu_color = Color(255,0,0);
					text = ">";
					text.append(child->name);
					text.append("<");
				}
				else
				{
					text = child->name;
				}

				int center = (kernel::instance()->parameters().render_width / 2);
				int font_width = font::measure_width( font, text() );
				font::draw_string( font, center-(font_width/2), height, text(), menu_color );
				height += VERTICAL_SPACING;

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
	// I only care about the down events
	if ( !event.is_down )
	{
		return;
	}
	
	bool is_up = (event.key == input::KEY_UP);
	bool is_down = (event.key == input::KEY_DOWN);
	bool is_arrow = is_up || is_down;
	bool should_advance = (event.key == input::KEY_RETURN || event.key == input::KEY_SPACE);
		
	if ( (!is_arrow) && (event.key == input::KEY_ESCAPE) )
	{
//		skip_screen( app );
		kernel::instance()->set_active(false);
	}
	else if ( should_advance )
	{
		// advance the screen to the selected menu
//		menunav.navigate_to_child(current_menu);
		MenuItem * item = menunav.child_at_index(current_menu);
		if ( item )
		{
			int * id = static_cast<int*>(item->userdata);
			if (id && *id == MENU_PLAY)
			{
				this->skip_screen(app);
			}
			else if (id && *id == MENU_QUIT)
			{
				kernel::instance()->set_active(false);
			}
		}
	}
	else
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
		LOGV( "Determine if mouse clicked on item\n" );
		//skip_screen( app );
	}
}

void MenuScreen::on_event( kernel::TouchEvent & event, kernel::IApplication * app )
{
	skip_screen( app );
}
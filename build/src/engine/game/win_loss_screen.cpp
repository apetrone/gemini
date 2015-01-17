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
#include <gemini/typedefs.h>
#include <slim/xlog.h>
#include "font.h"
#include "win_loss_screen.h"
#include "engine.h"
#include "input.h"
#include "stackstring.h"
#include "gamescreen.h"

const int MENU_PLAY_AGAIN = 1;
const int MENU_QUIT = 2;

const int VERTICAL_START = 220;
const int VERTICAL_SPACING = 48;

void virtual_screen_to_pixels(float & tx, float & ty);
void pixels_to_virtual_screen(float & px, float & py);

WinLossScreen::WinLossScreen()
{
	menu_font = assets::fonts()->load_from_path( "fonts/default24" );
	title_font = assets::fonts()->load_from_path( "fonts/default48" );
	
	MenuItem * root = menunav.root_menu();
	MenuItem * item;
	item = root->add_child("Play Again");
	item->userdata = (void*)&MENU_PLAY_AGAIN;
	
	item = root->add_child("Quit");
	item->userdata = (void*)&MENU_QUIT;
	
	current_menu = 0;
}

void WinLossScreen::setup_loss_screen()
{
	screen_name = "LossScreen";
	title = "You FAILED";

}

void WinLossScreen::setup_win_screen()
{
	screen_name = "WinScreen";
	title = "You WON!";
}

void WinLossScreen::on_show( kernel::IApplication * app )
{
}

void WinLossScreen::on_hide( kernel::IApplication * app )
{
}

void WinLossScreen::on_draw( kernel::IApplication * app )
{
	// draw title
	float vx, vy;
	
	float width = font::measure_width(title_font, title);
	float px = width/2.0;
	float py = 0;

	pixels_to_virtual_screen(px, py);
	
	vx = 0.5f - px;
	vy = 0.16f;
	virtual_screen_to_pixels(vx, vy);
	font::draw_string(title_font, vx, vy, title, Color(255,0,255));

#if 1
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
				int font_width = font::measure_width( menu_font, text() );
				font::draw_string( menu_font, center-(font_width/2), height, text(), menu_color );
				height += VERTICAL_SPACING;

			}
		}
	}
#endif
}

void WinLossScreen::on_update( kernel::IApplication * app ) {}

void WinLossScreen::on_step( kernel::IApplication * app ) {}

	
const char * WinLossScreen::name() const
{
	return screen_name;
}

void WinLossScreen::skip_screen( kernel::IApplication * app )
{
	engine::engine()->screen_controller()->pop_screen( app );
}

// any event that happens during the logo screen triggers a skip to the next screen
void WinLossScreen::on_event( kernel::KeyboardEvent & event, kernel::IApplication * app )
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
			if (id && *id == MENU_PLAY_AGAIN)
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

void WinLossScreen::on_event( kernel::MouseEvent & event, kernel::IApplication * app )
{
	// except mouse moved events.
	if ( event.subtype != kernel::MouseMoved )
	{
		LOGV( "Determine if mouse clicked on item\n" );
		//skip_screen( app );
	}
}

void WinLossScreen::on_event( kernel::TouchEvent & event, kernel::IApplication * app )
{
	skip_screen( app );
}
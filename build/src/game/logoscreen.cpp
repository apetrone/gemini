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
#include <slim/xlog.h>
#include "font.h"
#include "logoscreen.h"
#include "engine.h"

LogoScreen::LogoScreen()
{
	font = assets::fonts()->load_from_path( "fonts/default64" );
}

void LogoScreen::on_show( kernel::IApplication * app )
{
	LOGV( "LogoScreen on show\n" );
}

void LogoScreen::on_hide( kernel::IApplication * app )
{
	LOGV( "LogoScreen on hide\n" );
}

void LogoScreen::on_draw( kernel::IApplication * app )
{
	const char text[] = "LogoScreen";
	int center = (kernel::instance()->parameters().render_width / 2);
	int font_width = font::measure_width( font, text );
	font::draw_string( font, center-(font_width/2), 150, text, Color(255,255,255) );
}

void LogoScreen::on_update( kernel::IApplication * app ) {}

void LogoScreen::on_step( kernel::IApplication * app ) {}

	
const char * LogoScreen::name() const
{
	return "LogoScreen";
}

void LogoScreen::skip_screen( kernel::IApplication * app )
{
	engine::engine()->screen_controller()->pop_screen( app );
}

// any event that happens during the logo screen triggers a skip to the next screen
void LogoScreen::on_event( kernel::KeyboardEvent & event, kernel::IApplication * app )
{
	if ( event.is_down )
	{
		skip_screen( app );
	}
}

void LogoScreen::on_event( kernel::MouseEvent & event, kernel::IApplication * app )
{
	// except mouse moved events.
	if ( event.subtype != kernel::MouseMoved )
	{
		skip_screen( app );
	}
}

void LogoScreen::on_event( kernel::TouchEvent & event, kernel::IApplication * app )
{
	skip_screen( app );
}
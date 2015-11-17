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

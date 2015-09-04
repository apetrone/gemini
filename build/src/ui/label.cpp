// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "ui/label.h"
#include "ui/renderer.h"
#include "ui/compositor.h"

namespace gui
{
	Label::Label(Panel* parent) : Panel(parent)
	{
	}
	
	void Label::update(Compositor* compositor, const TimeState& timestate)
	{
		Panel::update(compositor, timestate);
	} // update
	
	void Label::render(Rect& frame, Compositor* compositor, Renderer* renderer, Style* style)
	{
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			background_color
		);
		
		gui::Rect bounds = frame;
		
		bounds.origin.x += 10;
		bounds.origin.y += 15;
		render_commands.add_font(font_handle, this->text.c_str(), bounds, foreground_color);
	}
		
	void Label::set_font(Compositor* compositor, const char* path)
	{
		gui::FontResult result = compositor->renderer->font_create(path, font_handle);
		if (result != gui::FontResult_Success)
		{
			// error loading font
		}
	}
	
	void Label::set_text(const std::string& text)
	{
		this->text = text;
	}
} // namespace gui

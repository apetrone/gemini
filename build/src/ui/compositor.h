/*
Copyright (c) 2011, <Adam Petrone>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "ui/panel.h"
#include "ui/events.h"

namespace gui
{
	class Style;
	class Renderer;
	class Listener;
		
	class Compositor : public Panel
	{
	public:
		ScreenInt width;
		ScreenInt height;
		Panel* focus;
		Panel* hot;
		Panel* capture;
		PanelVector zsorted;
		Style* style;
		Renderer* renderer;
		Point last_cursor;
		size_t next_z_depth;
		float aspect_ratio;
		Listener* listener;
		float update_interval_seconds;
		
		EventArgs queue[16];
		uint16_t next_message;
		uint32_t key_modifiers;
		
		TimeState timestate;

		Array<render::CommandList*> command_stream;
		
		LIBRARY_EXPORT Compositor(ScreenInt width, ScreenInt height);
		LIBRARY_EXPORT virtual ~Compositor();
		
		/// @desc Update the children given delta seconds.
		/// The compositor maintains its own time, so this should be called
		/// each frame.
		LIBRARY_EXPORT void update(float delta_seconds);
		
		// render this compositor and all children
		LIBRARY_EXPORT void render();
		
		// panel accessors
		LIBRARY_EXPORT Panel* get_focus() { return this->focus; }
		LIBRARY_EXPORT void set_focus(Panel* panel) { this->focus = panel; }
		
		LIBRARY_EXPORT Panel* get_hot() { return this->hot; }
		LIBRARY_EXPORT void set_hot(Panel* panel) { this->hot = panel; }
		
		LIBRARY_EXPORT Panel* get_capture() { return this->capture; }
		LIBRARY_EXPORT void set_capture(Panel* panel) { this->capture = panel; }
		
		LIBRARY_EXPORT void set_renderer(Renderer* renderer);
		LIBRARY_EXPORT void set_style(Style* style);
		LIBRARY_EXPORT Style* get_style() const;
		
		LIBRARY_EXPORT void send_to_front(Panel* panel);
		LIBRARY_EXPORT void sort_zorder(Panel* panel);
		
		LIBRARY_EXPORT virtual void add_child(Panel* panel);
		LIBRARY_EXPORT virtual void remove_child(Panel* panel);
		
		// events
		LIBRARY_EXPORT void cursor_move_absolute(ScreenInt x, ScreenInt y);
		LIBRARY_EXPORT void cursor_button(CursorButton::Type button, bool is_down);
		LIBRARY_EXPORT void cursor_scroll(uint16_t direction);
		LIBRARY_EXPORT void key_event(uint32_t unicode, bool is_down, uint32_t character, uint16_t modifiers);
		
		LIBRARY_EXPORT void resize(ScreenInt width, ScreenInt height);
		
		LIBRARY_EXPORT Panel* find_panel_at_point(const Point& point);
		LIBRARY_EXPORT Panel* find_deepest_panel_point(Panel* root, const Point& point);
		
		LIBRARY_EXPORT virtual void set_listener(Listener* listener);
		LIBRARY_EXPORT virtual void queue_event(const EventArgs& args);
		LIBRARY_EXPORT virtual void process_events();
		
		LIBRARY_EXPORT virtual void queue_commandlist(render::CommandList* const commandlist);
	}; // struct Compositor
	
} // namespace gui

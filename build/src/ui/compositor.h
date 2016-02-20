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
#pragma once

#include "ui/panel.h"
#include "ui/events.h"

namespace gui
{
	class Renderer;
	class Listener;

	namespace render
	{
		struct Vertex;
		struct CommandList;
	}

	class Compositor : public Panel
	{
	public:
		ScreenInt width;
		ScreenInt height;
		Panel* focus;
		Panel* hot;
		Panel* capture;
		PanelVector zsorted;

		Point last_cursor;
		size_t next_z_depth;
		Listener* listener;

		EventArgs queue[16];
		uint16_t next_message;
		uint32_t key_modifiers;

		render::CommandList command_list;

		Compositor(ScreenInt width, ScreenInt height, ResourceCache* cache, Renderer* renderer);
		virtual ~Compositor();

		/// @desc Update the children given delta seconds.
		/// The compositor maintains its own time, so this should be called
		/// each frame.
		void tick(float delta_seconds);

		// render this compositor and all children
		void draw();

		// panel accessors
		Panel* get_focus() { return this->focus; }
		void set_focus(Panel* panel) { this->focus = panel; }

		Panel* get_hot() { return this->hot; }
		void set_hot(Panel* panel) { this->hot = panel; }

		Panel* get_capture() { return this->capture; }
		void set_capture(Panel* panel) { this->capture = panel; }

		Renderer* get_renderer() const { return renderer; }

		void send_to_front(Panel* panel);
		void sort_zorder(Panel* panel);

		virtual void add_child(Panel* panel);
		virtual void remove_child(Panel* panel);

		ResourceCache* get_resource_cache() const { return resource_cache; }

		// events
		void cursor_move_absolute(ScreenInt x, ScreenInt y);
		void cursor_button(CursorButton::Type button, bool is_down);
		void cursor_scroll(int32_t direction);
		void key_event(uint32_t unicode, bool is_down, uint32_t character, uint16_t modifiers);

		void resize(ScreenInt width, ScreenInt height);

		// location is in compositor coordinates with the origin (0, 0) in the upper left
		Panel* find_panel_at_location(const Point& location, uint32_t flags);
		Panel* find_deepest_panel_at_location(Panel* root, const Point& location, uint32_t flags);

		virtual void set_listener(Listener* listener);
		virtual void queue_event(const EventArgs& args);
		virtual void process_events();

	private:
		void find_new_hot(ScreenInt dx, ScreenInt dy);

		Array<render::Vertex> vertex_buffer;
		Array<render::Vertex>* get_vertex_buffer() { return &vertex_buffer; }

		ResourceCache* resource_cache;
		Renderer* renderer;
	}; // struct Compositor

} // namespace gui

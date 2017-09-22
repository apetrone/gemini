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

#include <core/typespec.h>

namespace gui
{
	class Renderer;

	namespace render
	{
		struct Vertex;
		struct CommandList;
	}

	class EventFilter
	{
	public:
		// Returns true if the event should propagate forward.
		// Returns false if the event should not.
		virtual bool event_can_propagate(Panel* target, EventArgs& event) = 0;
	};

	class Compositor : public Panel
	{
		TYPESPEC_DECLARE_CLASS(Compositor, Panel);
	public:
		Panel* focus;
		Panel* hot;
		Panel* capture;
		Panel* drop_target;
		CursorButton::Type capture_button;


		Point last_cursor;
		size_t next_z_depth;

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
		Panel* get_focus() { return focus; }
		void set_focus(Panel* panel);

		Panel* get_hot() { return hot; }
		void set_hot(Panel* panel);

		Panel* get_capture() { return capture; }
		void set_capture(Panel* panel, CursorButton::Type button);

		const Point& get_cursor_position() const { return last_cursor; }

		Renderer* get_renderer() const { return renderer; }

		void send_to_front(Panel* panel);

		virtual void add_child(Panel* panel);
		virtual void remove_child(Panel* panel);

		ResourceCache* get_resource_cache() const { return resource_cache; }

		// events: returns true if event was handled by a Panel.
		bool cursor_move_absolute(ScreenInt x, ScreenInt y);
		bool cursor_button(CursorButton::Type button, bool is_down);
		void cursor_scroll(int32_t direction);
		void key_event(bool is_down, uint32_t key, uint16_t modifiers);
		void text_event(uint32_t unicode);

		void resize(ScreenInt width, ScreenInt height);
		void resize(const Size&) {}

		// location is in compositor coordinates with the origin (0, 0) in the upper left
		Panel* find_panel_at_location(const Point& location, uint32_t flags, Panel* ignore = nullptr);
		Panel* find_deepest_panel_at_location(Panel* root, const Point& location, uint32_t flags, Panel* ignore = nullptr);

		// install an event filter on the compositor
		void install_event_filter(EventFilter* filter);

		// remove an existing event filter on the compositor
		void remove_event_filter();

	private:
		bool find_new_hot(ScreenInt dx, ScreenInt dy);

		// Returns whether or not the event was handled by the panel
		bool dispatch_event_to_panel(Panel* panel, EventArgs& args);

		// Dispatch an event recursively from compositor to Panel.
		// Returns whether or not the event was handled.
		bool dispatch_recursive(Panel* panel, EventArgs& args);

		Array<render::Vertex> vertex_buffer;
		Array<render::Vertex>* get_vertex_buffer() { return &vertex_buffer; }

		ResourceCache* resource_cache;
		Renderer* renderer;
		EventFilter* event_filter;
	}; // struct Compositor

} // namespace gui

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
#include "ui/compositor.h"
#include "ui/renderer.h"
#include <core/logging.h>

#include <algorithm>

namespace gui
{
	struct ZSort_Panel_Descending
	{
		bool operator()(Panel* left, Panel* right)
		{
			// This must obey strict weak ordering; so we'll have to come up
			// with another plan.
			return (left->z_depth < right->z_depth);
		}
	}; // ZSort_Panel_Descending

	struct ZSort_Panel_AlwaysOnTop_Descending
	{
		bool operator()(Panel* left, Panel* right)
		{
			return (left->get_flags() & Panel::Flag_AlwaysOnTop) && !(right->get_flags() & Panel::Flag_AlwaysOnTop);
		}
	}; // ZSort_Panel_AlwaysOnTop_Descending

	Compositor::Compositor(ScreenInt width, ScreenInt height, ResourceCache* cache, Renderer* renderer) :
		Panel(nullptr),
		command_list(this, &vertex_buffer),
		resource_cache(cache),
		renderer(renderer)
	{
		size.width = width;
		size.height = height;
		set_origin(0, 0);

		last_cursor = Point(0, 0);

		focus = nullptr;
		hot = nullptr;
		capture = nullptr;
		drop_target = nullptr;

		set_hot(0);
		set_capture(0, CursorButton::None);

		next_z_depth = 0;
		listener = 0;

		key_modifiers = 0;

		next_message = 0;

		// We cannot start without a valid resource cache
		assert(cache);

		renderer->startup(this);

		set_name("compositor");
	} // Compositor

	Compositor::~Compositor()
	{
		if (renderer)
		{
			renderer->shutdown( this );
		}
	} // ~Compositor

	void Compositor::tick(float delta_seconds)
	{
		for(PanelVector::reverse_iterator it = zsorted.rbegin(); it != zsorted.rend(); ++it)
		{
			Panel* panel = (*it);
			panel->update(this, delta_seconds);
		}

		process_events();
		flags &= ~Flag_TransformIsDirty;
	} // update

	void Compositor::draw()
	{
		assert(renderer);

		command_list.reset();
		vertex_buffer.resize(0);

		renderer->begin_frame(this);
		for(PanelVector::reverse_iterator it = zsorted.rbegin(); it != zsorted.rend(); ++it)
		{
			Panel* panel = (*it);
			if (panel->is_visible())
			{
				panel->render(this, renderer, command_list);
			}
		}
		renderer->draw_commands(&command_list, vertex_buffer);
		renderer->end_frame();
	} // draw

	void Compositor::set_focus(Panel* panel)
	{
		EventArgs focus_args(this, Event_FocusLost);
		focus_args.focus = panel;
		focus_args.hot = panel;
		focus_args.capture = panel;
		// TODO: Can panels be focused with other buttons?
		focus_args.cursor_button = CursorButton::Left;
		focus_args.cursor = last_cursor;
		focus_args.sender = this;
		focus_args.target = focus;

		if (focus != panel)
		{
			if (focus)
			{
				focus_args.type = Event_FocusLost;
				focus->handle_event(focus_args);
			}

			if (panel)
			{
				focus_args.type = Event_FocusGain;
				focus_args.target = panel;
				panel->handle_event(focus_args);
			}
		}

		focus = panel;
	}

	void Compositor::send_to_front(Panel* panel)
	{
		if (panel && panel->can_send_to_front())
		{
			sort_zorder(panel);
		}
	} // send_to_front

	void Compositor::sort_zorder(Panel* panel)
	{
		if (!panel)
		{
			return;
		}

		size_t old_z = panel->z_depth;

		// search the zlist for the top window
		for( PanelVector::iterator it = zsorted.begin(); it != zsorted.end(); ++it )
		{
			Panel* target = (*it);

			if (target->z_depth == old_z)
			{
				target->z_depth = 0;
				break;
			}
			else
			{
				target->z_depth++;
			}
		}

		std::sort(zsorted.begin(), zsorted.end(), ZSort_Panel_Descending());
		std::sort(zsorted.begin(), zsorted.end(), ZSort_Panel_AlwaysOnTop_Descending());
	} // sort_zorder

	void Compositor::add_child(Panel* panel)
	{
		Panel::add_child(panel);

		zsorted.push_back(panel);
		panel->z_depth = next_z_depth++;
		sort_zorder(panel);
	} // add_child

	void Compositor::remove_child(Panel* panel)
	{
		Panel::remove_child(panel);

		PanelVector::iterator it, end;
		it = zsorted.begin();
		end = zsorted.end();

		for(; it != end; ++it)
		{
			if ((*it) == panel)
			{
				zsorted.erase(it);
				break;
			}
		}
	} // remove_child

	bool Compositor::find_new_hot(ScreenInt dx, ScreenInt dy)
	{
		Panel* last_hot = hot;

		Point cursor(last_cursor.x, last_cursor.y);
		Panel* newhot = get_capture();

		bool event_handled = false;

		if (!get_capture())
		{
			// There is no captured panel:
			// - moving the mouse tries to find a new hot panel
			newhot = find_panel_at_location(cursor, Panel::Flag_CursorEnabled | Panel::Flag_IsVisible);
			hot = newhot;

			if (newhot)
			{
				// if hot changed
				if (hot != last_hot)
				{
					if (hot && listener)
					{
						listener->hot_changed(last_hot, hot);
					}

					if (last_hot)
					{
						// mouse exit
						EventArgs args(this, Event_CursorExit);
						args.cursor = cursor;
						args.hot = last_hot;
						args.focus = get_focus();
						args.capture = get_capture();
						args.delta.x = dx;
						args.delta.y = dy;
						args.local = last_hot->compositor_to_local(cursor);
						args.sender = this;
						args.target = last_hot;
						last_hot->handle_event(args);
						event_handled = event_handled || args.handled;
					}

					if (hot)
					{
						// mouse enter
						EventArgs args(this, Event_CursorEnter);
						args.cursor = cursor;
						args.hot = hot;
						args.focus = get_focus();
						args.capture = get_capture();
						args.delta.x = dx;
						args.delta.y = dy;
						args.local = hot->compositor_to_local(cursor);
						args.sender = this;
						args.target = hot;
						hot->handle_event(args);
						event_handled = event_handled || args.handled;
					}
				}
			}

		}

		if (get_capture())
		{
			// There is a captured panel.
			// - A drag event is happening to the capture.
			// - A drag and drop event from capture to... drop_target.
			Panel* target = get_capture();

			EventArgs args(this, Event_CursorDrag);
			args.cursor_button = capture_button;
			args.cursor = cursor;
			args.hot = focus;
			args.focus = get_focus();
			args.capture = get_capture();
			args.delta.x = dx;
			args.delta.y = dy;
			args.local = focus->compositor_to_local(cursor);
			args.sender = this;
			args.target = get_capture();

			if (target)
			{
				target->handle_event(args);
				event_handled = event_handled || args.handled;
			}

			Panel* last_drop_target = drop_target;

			// try to find a drop target
			drop_target = find_panel_at_location(cursor, Panel::Flag_CursorEnabled | Panel::Flag_IsVisible | Panel::Flag_CanDrop, get_capture());
			if (drop_target != last_drop_target)
			{
				if (last_drop_target)
				{
					EventArgs hoverargs(this, Event_CursorDragExit);
					hoverargs.cursor_button = capture_button;
					hoverargs.cursor = cursor;
					hoverargs.hot = last_drop_target;
					hoverargs.focus = get_focus();
					hoverargs.delta.x = dx;
					hoverargs.delta.y = dy;
					hoverargs.local = last_drop_target->compositor_to_local(cursor);
					hoverargs.sender = this;
					hoverargs.target = last_drop_target;
					last_drop_target->handle_event(hoverargs);
					event_handled = event_handled || hoverargs.handled;
				}

				if (drop_target)
				{
					EventArgs hoverargs(this, Event_CursorDragEnter);
					hoverargs.cursor_button = capture_button;
					hoverargs.cursor = cursor;
					hoverargs.hot = drop_target;
					hoverargs.focus = get_focus();
					hoverargs.delta.x = dx;
					hoverargs.delta.y = dy;
					hoverargs.local = drop_target->compositor_to_local(cursor);
					hoverargs.sender = this;
					hoverargs.target = drop_target;
					drop_target->handle_event(hoverargs);
					event_handled = event_handled || hoverargs.handled;
				}
			}
		}
		else if (hot)
		{
			EventArgs args(this, Event_CursorMove);
			args.cursor = cursor;
			args.hot = hot;
			args.focus = get_focus();
			args.capture = get_capture();
			args.delta.x = dx;
			args.delta.y = dy;
			args.local = hot->compositor_to_local(cursor);
			args.sender = this;
			args.target = hot;
			hot->handle_event(args);
			event_handled = event_handled || args.handled;
		}

		return event_handled;
	} // find_new_hot

	bool Compositor::cursor_move_absolute(ScreenInt x, ScreenInt y)
	{
		ScreenInt dx = static_cast<ScreenInt>(x - last_cursor.x);
		ScreenInt dy = static_cast<ScreenInt>(y - last_cursor.y);

		if (dx != 0 || dy != 0)
		{
			last_cursor.x = x;
			last_cursor.y = y;

			return find_new_hot(dx, dy);
		} // any mouse delta movement

		return false;
	} // cursor_move_absolute

	void Compositor::cursor_button( CursorButton::Type button, bool is_down )
	{
		if (is_down)
		{
			// We must report the focus panel has lost focus FIRST.
			Panel* panel = get_hot();

			// Then we have to dispatch the button press to the new panel.
			EventArgs args(this, Event_CursorButtonPressed);
			args.focus = get_focus();
			args.hot = panel;
			args.capture = panel;
			args.cursor_button = button;
			args.cursor = last_cursor;
			args.sender = this;

			if (panel)
			{
				args.target = panel;
				args.local = panel->compositor_to_local(last_cursor);
				panel->handle_event(args);
			}

			// Lastly, can dispatch the FocusGain message to the new panel.
			if (panel != focus)
			{
				set_focus(panel);
			}


			set_hot(panel);
			set_capture(panel, button);

			send_to_front(panel);
		}
		else
		{
			EventArgs args(this, Event_CursorButtonReleased);
			args.focus = get_focus();
			args.hot = get_hot();
			args.capture = get_capture();
			args.cursor_button = button;
			assert(button != CursorButton::None);
			args.cursor = last_cursor;
			args.sender = this;


			if (args.focus)
			{
				args.target = get_focus();
				args.local = args.focus->compositor_to_local(last_cursor);
				args.focus->handle_event(args);
			}

			if (get_capture())
			{
				args.target = get_capture();
				args.type = Event_CursorExit;
				args.capture = nullptr;
				get_capture()->handle_event(args);
			}

			if (drop_target)
			{
				// dispatch drop event
				args.target = drop_target;
				args.type = Event_CursorDrop;
				args.capture = get_capture();
				args.local = drop_target->compositor_to_local(last_cursor);
				drop_target->handle_event(args);
			}

			set_capture(0, CursorButton::None);
			find_new_hot(0, 0);
			drop_target = nullptr;
		}
	} // cursor_button

	void Compositor::cursor_scroll(int32_t direction)
	{
		EventArgs args(this, Event_CursorScroll);
		args.focus = get_focus();
		args.hot = get_hot();
		args.capture = get_capture();
		args.cursor = last_cursor;
		args.modifiers = key_modifiers;
		args.wheel = direction;
		args.sender = this;

		Panel* panel = args.hot;
		if (panel)
		{
			args.target = panel;
			args.local = panel->compositor_to_local(last_cursor);
			panel->handle_event(args);
		}
	} // cursor_scroll

	void Compositor::key_event(uint32_t /*unicode*/, bool is_down, uint32_t /*character*/, uint16_t modifiers)
	{
		key_modifiers = modifiers;

		EventArgs args(this, is_down ? Event_KeyButtonPressed : Event_KeyButtonReleased);
		args.focus = get_focus();
		args.hot = get_hot();
		args.capture = get_capture();
		args.cursor = last_cursor;
		args.modifiers = key_modifiers;
		args.sender = this;

		// key events are directed to the panel in focus
		Panel* panel = args.focus;
		if (panel)
		{
			args.target = panel;
			args.local = panel->compositor_to_local(last_cursor);
			panel->handle_event(args);
		}
	} // key_event

	void Compositor::resize(ScreenInt new_width, ScreenInt new_height)
	{
		size.width = new_width;
		size.height = new_height;

		// force transform update for all children
		flags |= Flag_TransformIsDirty;
	} // resize

	bool hit_test_panel(Panel* panel, const Point& location, uint32_t option_flags)
	{
		assert(panel != nullptr);

		// TODO: convert compositor coordinates to panel local coordinates
		const Point local_coords = panel->compositor_to_local(location);
		const bool passed_flags = panel->has_flags(option_flags);
		const bool hit_test = panel->hit_test_local(local_coords);
		const bool result = passed_flags && hit_test;
//		if (!result)
//		{
//			fprintf(stdout, "[%s] local_coords = %2.2f, %2.2f [flags: %s, hit_test: %s]\n",
//				panel->get_name(),
//				local_coords.x,
//				local_coords.y,
//				passed_flags ? "Yes" : "No",
//				hit_test ? "Yes" : "No"
//			);
//		}

		return result;
	}

	Panel* Compositor::find_panel_at_location(const Point& location, uint32_t option_flags, Panel* ignore)
	{
		// This must go front to back traversal to find panels.
		for(PanelVector::iterator it = zsorted.begin(); it != zsorted.end(); ++it)
		{
			Panel* hit_panel = find_deepest_panel_at_location((*it), location, option_flags, ignore);
			if (hit_panel)
			{
				return hit_panel;
			}
		}

		return nullptr;
	} // find_panel_at_location

	Panel* Compositor::find_deepest_panel_at_location(Panel* root, const gui::Point& location, uint32_t option_flags, Panel* ignore)
	{
		if (!root)
		{
			return 0;
		}

		if (hit_test_panel(root, location, option_flags) && root != ignore)
		{
			// hit test passed for root; descend into children
			Panel* cpanel = 0;
			for (PanelVector::iterator child = root->children.begin(); child != root->children.end(); ++child)
			{
				cpanel = (*child);
				if (hit_test_panel(cpanel, location, option_flags) && cpanel != ignore)
				{
					return find_deepest_panel_at_location(cpanel, location, option_flags, ignore);
				}
			}

			return root;
		}
		else
		{
			return nullptr;
		}
	} // find_deepest_panel_point

	void Compositor::set_listener(Listener* event_listener)
	{
		listener = event_listener;
	} // set_listener

	void Compositor::queue_event(const EventArgs& args)
	{
		if (next_message == 15)
		{
			fprintf(stderr, "Overflow event queue! Ignoring event\n");
			return;
		}

		queue[next_message] = args;


		++next_message;
	}

	void Compositor::process_events()
	{
		next_message = 0;
		if (listener == 0)
		{
			// no one to listen to our events!
			return;
		}

		for (uint16_t i = 0; i < 16; ++i)
		{
			EventArgs& event = queue[i];
			if (event.type != Invalid)
			{
				listener->handle_event(event);
				event.type = Invalid;
			}
		}
	}
} // namespace gui

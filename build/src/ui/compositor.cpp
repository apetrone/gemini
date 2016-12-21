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
#include <ui/layout.h>
#include <core/logging.h>

#include <algorithm> // for std::sort

TYPESPEC_REGISTER_CLASS(gui::Compositor);

namespace gui
{
	struct ZSort_Panel_AlwaysOnTop_Descending
	{
		bool operator()(Panel* left, Panel* right)
		{
			return (left->get_flags() & Panel::Flag_AlwaysOnTop) && !(right->get_flags() & Panel::Flag_AlwaysOnTop);
		}
	}; // ZSort_Panel_AlwaysOnTop_Descending

	Compositor::Compositor(ScreenInt width, ScreenInt height, ResourceCache* cache, Renderer* renderer)
		: Panel(nullptr)
		, vertex_buffer(gui_allocator())
		, command_list(gui_allocator(), this, &vertex_buffer)
		, resource_cache(cache)
		, renderer(renderer)
		, event_filter(nullptr)
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

		key_modifiers = 0;

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

		flags &= ~Flag_TransformIsDirty;
	} // update

	void Compositor::draw()
	{
		assert(renderer);

		command_list.reset();
		vertex_buffer.resize(0);

		renderer->begin_frame(this);

		render_children(this, renderer, command_list);

		// draw hot
		//if (hot)
		//{
		//	hot->render_geometry(command_list, gemini::Color(1.0f, 0.0f, 0.0f, 0.5f));
		//}

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
				focus_args.target = focus;
				dispatch_recursive(focus, focus_args);
			}

			if (panel)
			{
				focus_args.handled = false;
				focus_args.type = Event_FocusGain;
				focus_args.target = panel;
				dispatch_recursive(panel, focus_args);
			}
		}

		focus = panel;
	}

	void Compositor::set_hot(Panel* panel)
	{
		//if (panel)
		//{
		//	LOGV("set hot to %s\n", panel->get_name());
		//}
		//else
		//{
		//	LOGV("set hot to NULL\n");
		//}
		hot = panel;
	}

	void Compositor::set_capture(Panel* panel, CursorButton::Type button)
	{
		//if (panel)
		//{
		//	LOGV("set capture panel to %s\n", panel->get_name());
		//}
		//else
		//{
		//	LOGV("set capture panel to NULL\n");
		//}
		capture = panel;
		capture_button = button;
	}

	void Compositor::send_to_front(Panel* panel)
	{
		if (panel && panel->can_send_to_front())
		{
			zsort_children(panel);
			std::sort(zsorted.begin(), zsorted.end(), ZSort_Panel_AlwaysOnTop_Descending());
		}
	} // send_to_front

	void Compositor::add_child(Panel* panel)
	{
		Panel::add_child(panel);
		std::sort(zsorted.begin(), zsorted.end(), ZSort_Panel_AlwaysOnTop_Descending());
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
	} // hit_test_panel

	bool Compositor::find_new_hot(ScreenInt dx, ScreenInt dy)
	{
		Point cursor(last_cursor.x, last_cursor.y);
		Panel* newhot = nullptr;

		bool event_handled = false;

		const uint32_t panel_flags = (Panel::Flag_CursorEnabled | Panel::Flag_IsVisible);

		if (!get_capture())
		{
			// There is no captured panel:
			// - moving the mouse tries to find a new hot panel

			newhot = find_panel_at_location(cursor, panel_flags);

			if (newhot != hot)
			{
				if (hot)
				{
					// mouse exit
					EventArgs args(this, Event_CursorExit);
					args.cursor = cursor;
					args.hot = hot;
					args.focus = get_focus();
					args.capture = get_capture();
					args.delta.x = dx;
					args.delta.y = dy;
					args.sender = this;
					args.target = hot;
					event_handled = dispatch_recursive(hot, args) || event_handled;
				}

				if (newhot)
				{
					// if hot changed
					// mouse enter
					EventArgs args(this, Event_CursorEnter);
					args.cursor = cursor;
					args.hot = newhot;
					args.focus = get_focus();
					args.capture = get_capture();
					args.delta.x = dx;
					args.delta.y = dy;
					args.sender = this;
					args.target = newhot;
					event_handled = dispatch_recursive(newhot, args) || event_handled;
				}
			}

			set_hot(newhot);
		}

		if (get_capture())
		{
			newhot = find_panel_at_location(cursor, panel_flags);

			// There is a captured panel.
			// - A drag event is happening to the capture.
			// - A drag and drop event from capture to... drop_target.
//			Panel* target = get_capture();

			EventArgs args(this, Event_CursorDrag);
			args.cursor_button = capture_button;
			args.cursor = cursor;
			args.hot = focus;
			args.focus = get_focus();
			args.capture = get_capture();
			args.delta.x = dx;
			args.delta.y = dy;
			args.sender = this;
			args.target = get_capture();
			event_handled = dispatch_recursive(get_capture(), args) || event_handled;

			if (drop_target)
			{
				EventArgs args(this, Event_CursorDropMove);
				args.cursor_button = capture_button;
				args.cursor = cursor;
				args.hot = focus;
				args.focus = get_focus();
				args.capture = get_capture();
				args.delta.x = dx;
				args.delta.y = dy;
				args.sender = this;
				args.target = drop_target;
				event_handled = dispatch_recursive(drop_target, args) || event_handled;
			}

			Panel* last_drop_target = drop_target;

			// try to find a drop target
			drop_target = find_panel_at_location(cursor, Panel::Flag_CursorEnabled | Panel::Flag_IsVisible | Panel::Flag_CanDrop, get_capture());
			if (drop_target != last_drop_target)
			{
				if (last_drop_target)
				{
					EventArgs hoverargs(this, Event_CursorDropExit);
					hoverargs.cursor_button = capture_button;
					hoverargs.cursor = cursor;
					hoverargs.hot = last_drop_target;
					hoverargs.focus = get_focus();
					hoverargs.delta.x = dx;
					hoverargs.delta.y = dy;
					hoverargs.sender = this;
					hoverargs.target = last_drop_target;
					event_handled = dispatch_recursive(last_drop_target, args) || event_handled;
				}

				if (drop_target)
				{
					EventArgs hoverargs(this, Event_CursorDropEnter);
					hoverargs.cursor_button = capture_button;
					hoverargs.cursor = cursor;
					hoverargs.hot = drop_target;
					hoverargs.focus = get_focus();
					hoverargs.delta.x = dx;
					hoverargs.delta.y = dy;
					hoverargs.sender = this;
					hoverargs.target = drop_target;
					event_handled = dispatch_recursive(drop_target, args) || event_handled;
				}
			}

			if (newhot != hot)
			{
				if (hot)
				{
					// mouse exit
					EventArgs args(this, Event_CursorDragExit);
					args.cursor = cursor;
					args.hot = newhot;
					args.focus = get_focus();
					args.capture = get_capture();
					args.delta.x = dx;
					args.delta.y = dy;
					args.sender = this;
					args.target = hot;
					event_handled = dispatch_recursive(hot, args) || event_handled;
				}

				if (newhot)
				{
					// if hot changed
					// mouse enter
					EventArgs args(this, Event_CursorDragEnter);
					args.cursor = cursor;
					args.hot = newhot;
					args.focus = get_focus();
					args.capture = get_capture();
					args.delta.x = dx;
					args.delta.y = dy;
					args.sender = this;
					args.target = newhot;
					event_handled = dispatch_recursive(newhot, args) || event_handled;
				}

				hot = newhot;
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
			args.sender = this;
			args.target = hot;
			event_handled = dispatch_recursive(hot, args) || event_handled;
		}

		return event_handled;
	} // find_new_hot

	bool Compositor::dispatch_event_to_panel(Panel* panel, EventArgs& args)
	{
		//LOGV("-> dispatch %s to panel %s\n", event_type_to_string(args.type), panel->get_name());
		bool propagate_event = true;
		if (event_filter)
		{
			propagate_event = event_filter->event_can_propagate(panel, args);
		}

		if (propagate_event)
		{
			Layout* layout = panel->get_layout();
			if (layout)
			{
				layout->handle_event(args);
			}

			if (!args.handled)
			{
				panel->handle_event(args);
			}
		}

		return propagate_event && (!args.handled);
	} // dispatch_event_to_panel

	bool Compositor::dispatch_recursive(Panel* panel, EventArgs& args)
	{

		if (panel == this)
		{
			return false;
		}

		//if (panel->get_parent() == args.compositor)
		//{
		//	LOGV("---------------------\n");
		//}

		bool result = dispatch_recursive(panel->get_parent(), args);
		if (!result)
		{
			args.local = panel->compositor_to_local(args.cursor);

			bool propagate_event = true;
			if (event_filter)
			{
				propagate_event = event_filter->event_can_propagate(panel, args);
			}

			if (propagate_event)
			{
				Layout* layout = panel->get_layout();
				if (layout)
				{
					layout->handle_event(args);
				}

				if (!args.handled)
				{
					//LOGV("dispatch event to %s\n", panel->get_name());
					panel->handle_event(args);
				}
			}
			else
			{
				return true;
			}
		}

		return args.handled;
	} // dispatch_recursive

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

	void Compositor::cursor_button(CursorButton::Type button, bool is_down)
	{
		bool event_handled = false;
		if (is_down)
		{
			Panel* panel = get_hot();
			if (panel)
			{
				// Then we have to dispatch the button press to the new panel.
				EventArgs args(this, Event_CursorButtonPressed);
				args.focus = get_focus();
				args.hot = panel;
				args.capture = panel;
				args.cursor_button = button;
				args.cursor = last_cursor;
				args.sender = this;
				args.target = panel;
				event_handled = dispatch_recursive(panel, args) || event_handled;

				if (!event_handled)
				{
					if (panel != focus)
					{
						set_focus(panel);
					}
				}
			}
			else
			{
				if (panel != focus)
				{
					set_focus(panel);
				}
			}

			set_hot(panel);

			if (!get_capture())
			{
				if (panel && panel->point_in_capture_rect(panel->compositor_to_local(last_cursor)) && panel->has_flags(Flag_CanMove))
				{
					set_capture(panel, button);
				}
				else
				{
					set_capture(nullptr, button);
				}
			}

			// TODO: the event dispatch should re-order the panel!
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

			if (get_hot())
			{
				args.target = get_hot();
				event_handled = dispatch_recursive(get_hot(), args) || event_handled;
			}

			//if (get_capture())
			//{
			//	args.target = get_capture();
			//	args.type = Event_CursorExit;
			//	args.capture = nullptr;
			//	//dispatch_event_to_panel(get_capture(), args);
			//	dispatch_topdown_event(get_capture(), last_cursor, args, Flag_CursorEnabled);
			//}

			if (drop_target)
			{
				// dispatch drop event
				args.type = Event_CursorDrop;
				args.capture = get_capture();
				args.target = drop_target;
				event_handled = dispatch_recursive(drop_target, args) || event_handled;
			}

			set_capture(0, CursorButton::None);
			find_new_hot(0, 0);
			drop_target = nullptr;
		}

		// return event_handled;
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
		args.target = args.hot;

		Panel* panel = args.hot;
		if (panel)
		{
			dispatch_recursive(panel, args);
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
		args.target = args.focus;

		// key events are directed to the panel in focus
		Panel* panel = args.focus;
		if (panel)
		{
			dispatch_recursive(panel, args);
		}
	} // key_event

	void Compositor::resize(ScreenInt new_width, ScreenInt new_height)
	{
		size.width = new_width;
		size.height = new_height;

		// force transform update for all children
		flags |= Flag_TransformIsDirty;
	} // resize

	bool hit_test_layout(Panel* panel, const Point& location)
	{
		Layout* layout = panel->get_layout();
		if (layout == nullptr)
			return false;

		const Point local_coords = panel->compositor_to_local(location);
		const bool hit_layout = layout->hit_test_local(local_coords);
		LOGV("test panel's layout %s, result => %i\n", panel->get_name(), hit_layout);
		return hit_layout;
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

		return this;
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

	void Compositor::install_event_filter(EventFilter* filter)
	{
		event_filter = filter;
	} // install_event_filter

	void Compositor::remove_event_filter()
	{
		event_filter = nullptr;
	} // remove_event_filter

} // namespace gui

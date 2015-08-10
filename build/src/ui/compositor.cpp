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

#include <algorithm>

namespace gui
{
	struct ZSort_Panel_Descending
	{
		bool operator() (Panel * left, Panel * right)
		{
			return left->z_depth < right->z_depth;
		}
	}; // ZSort_Panel_Descending

	Compositor::Compositor( ScreenInt w, ScreenInt h ) : Panel(0)
	{
		this->width = w;
		this->height = h;
		this->renderer = 0;
		this->aspect_ratio = width / (float)height;

		this->last_cursor = Point(0, 0);
		
		this->set_focus(0);
		this->set_hot(0);
		this->set_capture(0);
		
		this->next_z_depth = 0;
		this->listener = 0;
		
		this->style = 0;
		this->key_modifiers = 0;
		
		update_interval_seconds = (1.0f/20.0f); // 20 times per second
		next_message = 0;
	} // Compositor
	
	Compositor::~Compositor()
	{
		if ( this->renderer )
		{
			this->renderer->shutdown( this );
		}
	} // ~Compositor
	
	void Compositor::update(float delta_seconds)
	{
		timestate.delta_seconds = delta_seconds;
		for(PanelVector::reverse_iterator it = zsorted.rbegin(); it != zsorted.rend(); ++it)
		{
			Panel* panel = (*it);
			panel->update(this, timestate);
		}

		process_events();
		
	} // update
	
	void Compositor::render()
	{
		if ( this->renderer == 0 )
		{
			return;
		}
		
		command_stream.resize(0);
		
		this->renderer->begin_frame( this );
		
		for( PanelVector::reverse_iterator it = zsorted.rbegin(); it != zsorted.rend(); ++it )
		{
			Panel * panel = (*it);
			if (panel->is_visible())
			{
				panel->render(panel->bounds, this, this->renderer, get_style());
			}
		}
		
		this->renderer->draw_command_lists(&command_stream[0], command_stream.size());
		
		this->renderer->end_frame();
	} // render
	
	
	void Compositor::set_renderer(Renderer* renderer)
	{
		this->renderer = renderer;
		this->renderer->startup( this );
	} // set_renderer
	
	void Compositor::set_style(Style* style)
	{
		this->style = style;
	} // set_style
	
	Style* Compositor::get_style() const
	{
		return this->style;
	} // get_style
	
	void Compositor::send_to_front(Panel* panel)
	{
		if (panel && panel->can_send_to_front())
		{
			sort_zorder(panel);
		}
	} // send_to_front
	
	void Compositor::sort_zorder(Panel* panel)
	{
		if ( !panel )
		{
			return;
		}
		
		size_t old_z = panel->z_depth;
		
		// search the zlist for the top window
		for( PanelVector::iterator it = zsorted.begin(); it != zsorted.end(); ++it )
		{
			Panel * p = (*it);
			
			if ( p->z_depth == old_z )
			{
				p->z_depth = 0;
				break;
			}
			else
			{
				p->z_depth++;
			}
		}
		
		
		std::sort( zsorted.begin(), zsorted.end(), ZSort_Panel_Descending() );
		
	} // sort_zorder
	
	void Compositor::add_child( Panel * panel )
	{
		Panel::add_child( panel );
		
		zsorted.push_back( panel );
		panel->z_depth = next_z_depth++;
		this->sort_zorder( panel );
	} // add_child
	
	void Compositor::remove_child( Panel * panel )
	{
		Panel::remove_child( panel );
		
		PanelVector::iterator it, end;
		it = zsorted.begin();
		end = zsorted.end();
		
		for( ; it != end; ++it )
		{
			if ( (*it) == panel )
			{
				zsorted.erase( it );
				break;
			}
		}
	} // remove_child
	
	void Compositor::cursor_move_absolute( ScreenInt x, ScreenInt y )
	{
		ScreenInt dx = (x - last_cursor.x);
		ScreenInt dy = (y - last_cursor.y);
		
		Panel * last_hot = this->hot;
		if ( dx != 0 || dy != 0 )
		{
			last_cursor.x = x;
			last_cursor.y = y;
			
			// reset hot and try to find a new one
			this->hot = 0;
			
			Point cursor( x, y );

			if ( !hot )
			{
				hot = find_panel_at_point( cursor );
				
				// if hot changed
				if ( hot != last_hot )
				{
					if (hot && listener)
					{
						listener->hot_changed(last_hot, hot);
					}
					
					if ( last_hot )
					{
						// mouse exit
						EventArgs args(this, Event_CursorExit);
						args.cursor = cursor;
						args.hot = last_hot;
						args.focus = get_focus();
						args.capture = get_capture();
						args.delta.x = dx;
						args.delta.y = dy;
						args.local = cursor - last_hot->bounds.origin;
						last_hot->handle_event(args);
					}
					
					if ( hot )
					{
						// mouse enter
						EventArgs args(this, Event_CursorEnter);
						args.cursor = cursor;
						args.hot = hot;
						args.focus = get_focus();
						args.capture = get_capture();
						args.delta.x = dx;
						args.delta.y = dy;
						args.local = cursor - hot->bounds.origin;
						hot->handle_event(args);
					}
				}
			}
			
			if ( get_capture() )
			{
				Panel * target = get_capture();
				
				EventArgs args( this, Event_CursorDrag );
				args.cursor = cursor;
				args.hot = focus;
				args.focus = get_focus();
				args.capture = get_capture();
				args.delta.x = dx;
				args.delta.y = dy;
				args.local = cursor - focus->bounds.origin;

				if ( target )
				{
					target->handle_event( args );
				}
				// mouse move
			}
			else if ( hot )
			{
				EventArgs args( this, Event_CursorMove );
				args.cursor = cursor;
				args.hot = hot;
				args.focus = get_focus();
				args.capture = get_capture();
				args.delta.x = dx;
				args.delta.y = dy;
				args.local = cursor - hot->bounds.origin;
				
				hot->handle_event( args );
			}
		} // any mouse delta movement
	} // cursor_move_absolute
	
	void Compositor::cursor_button( CursorButton::Type button, bool is_down )
	{
		if ( is_down )
		{
//			Panel * panel = find_panel_at_point( cursor );


			
			EventArgs args( this, Event_CursorButtonPressed );
			args.focus = get_focus();
			args.hot = get_hot();
			args.capture = get_capture();
			args.cursor_button = button;
			args.cursor = last_cursor;
			
			Panel * panel = get_hot();
			
			if ( panel )
			{
				args.local = last_cursor - panel->bounds.origin;
				panel->handle_event( args );
			}
			
			// capture
			this->set_focus( panel );
			this->set_hot( panel );
			this->set_capture(panel);

			send_to_front(panel);
		}
		else
		{
//			Panel * panel = find_panel_at_point( cursor );

			EventArgs args( this, Event_CursorButtonReleased );
			args.focus = get_focus();
			args.hot = get_hot();
			args.capture = get_capture();
			args.cursor_button = button;
			args.cursor = last_cursor;
			
			if ( args.focus )
			{
				args.local = last_cursor - args.focus->bounds.origin;
				args.focus->handle_event( args );
			}

			if (get_capture())
			{
				args.type = Event_CursorExit;
				args.capture = nullptr;
				get_capture()->handle_event(args);
			}
		
			this->set_capture(0);
		}
	} // cursor_button
	
	void Compositor::cursor_scroll( uint16_t direction )
	{
		
	} // cursor_scroll
	
	void Compositor::key_event(uint32_t unicode, bool is_down, uint32_t character, uint16_t modifiers)
	{
		key_modifiers = modifiers;
		
		EventArgs args(this, is_down ? Event_KeyButtonPressed : Event_KeyButtonReleased);
		args.focus = get_focus();
		args.hot = get_hot();
		args.capture = get_capture();
		args.cursor = last_cursor;
		args.modifiers = key_modifiers;
		
		// key events are directed to the panel in focus
		Panel* panel = args.focus;
		if (panel)
		{
			args.local = last_cursor - panel->bounds.origin;
			panel->handle_event(args);
		}
	} // key_event
	
	
	void Compositor::resize( ScreenInt width, ScreenInt height )
	{
		
	} // resize
	
	Panel * Compositor::find_panel_at_point( const Point & point )
	{
		Panel * panel = 0;
		Panel * closest_panel = 0;
		
		// reset hot and try to find a new one
		this->hot = 0;

		for( PanelVector::iterator it = zsorted.begin(); it != zsorted.end(); ++it )
		{
			panel = (*it);
			if (panel->has_flags(Panel::Flag_CursorEnabled) && panel->hit_test_local(point))
//			if ( panel->bounds.is_point_inside( point ) )
			{
				closest_panel = panel;
				return find_deepest_panel_point( panel, point );
			}
		}
		
		return closest_panel;
	} // find_panel_at_point
	
	Panel * Compositor::find_deepest_panel_point( Panel * root, const gui::Point & point )
	{
		if ( !root )
		{
			return 0;
		}
		if (root->has_flags(Panel::Flag_CursorEnabled) && root->hit_test_local(point))
//		if ( root->bounds.is_point_inside( point ) )
		{
			Panel * cpanel = 0;
			for ( PanelVector::iterator child = root->children.begin(); child != root->children.end(); ++child )
			{
				cpanel = (*child);
				if (cpanel->has_flags(Panel::Flag_CursorEnabled) && cpanel->hit_test_local(point))
//				if ( cpanel->bounds.is_point_inside( point ) )
				{
					return find_deepest_panel_point( cpanel, point );
				}
			}
			
			return root;
		}
		
		return 0;
	} // find_deepest_panel_point

	void Compositor::set_listener(Listener *listener)
	{
		this->listener = listener;
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
	
	void Compositor::queue_commandlist(render::CommandList* const commandlist)
	{
		command_stream.push_back(commandlist);
	}
	
} // namespace gui

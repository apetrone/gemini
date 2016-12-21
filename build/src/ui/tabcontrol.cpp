// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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
#include <ui/ui.h>
#include <ui/panel.h>
#include <ui/tabcontrol.h>
#include <ui/compositor.h>

#include <core/typedefs.h>
#include <core/logging.h>

TYPESPEC_REGISTER_CLASS(gui::TabControl);
TYPESPEC_REGISTER_CLASS(gui::TabControl::TabButton);

namespace gui
{
	void TabControl::TabButton::handle_event(EventArgs& args)
	{
		if (args.type == Event_CursorDrag)
		{
			LOGV("dragging tab button\n");
		}

		// Send this up the chain.
		//get_parent()->handle_event(args);

		if (!args.handled)
		{
			Panel::handle_event(args);
		}
	}

	void TabControl::TabButton::render(Compositor* compositor, Renderer* renderer, render::CommandList& render_commands)
	{
		if (get_panel()->is_visible())
		{
			// this tab is active.
			background_color = gemini::Color(0.5f, 0.7f, 1.0f);
		}
		else
		{
			background_color = gemini::Color(0.75f, 0.75f, 0.75f);
		}

		Button::render(compositor, renderer, render_commands);
	} // render

	TabControl::TabControl(Panel* root)
		: Panel(root)
		, active_tab(nullptr)
		, tabs(gui_allocator())
	{
		// TODO: should get this from style
		tab_size.width = 40;
		tab_size.height = 20;

		font = get_compositor()->get_resource_cache()->create_font("fonts/debug.ttf", 16);

		flags |= Flag_CanDrop;

		in_preview_mode = false;
	}

	void TabControl::handle_event(EventArgs& args)
	{
		if (args.type == Event_CursorDrop)
		{
			if (args.capture && args.capture->get_parent() != this && (args.capture->get_flags() & Flag_CanDrop))
			{
				Panel* panel = static_cast<Panel*>(args.capture);
				add_child(panel);
				add_tab(0, panel->get_name(), panel);
			}
		}
		else if (args.type == Event_CursorButtonPressed)
		{
			if (args.capture->typespec()->identifier() == TYPESPEC_IDENTIFIER("gui::TabControl::TabButton"))
			{
				TabButton* captured = static_cast<TabButton*>(args.capture);

				// show clicked on tab.
				{
					TabButton* tab = nullptr;
					size_t index = 0;
					for (TabButton* current : tabs)
					{
						if (captured == current)
						{
							tab = current;
							break;
						}
						++index;
					}

					if (tab)
					{
						show_tab(index);
						current_tab_index = index;
					}
				}

				last_position = args.cursor;
				tab_position = captured->get_origin();
				in_preview_mode = true;
				captured->bring_to_front();
			}
		}
		else if (args.type == Event_CursorButtonReleased)
		{
			in_preview_mode = false;

			if (args.target && args.target->typespec()->identifier() == TYPESPEC_IDENTIFIER("gui::TabControl::TabButton"))
			{
				current_tab_index = 0;
			}
		}
		else if (args.type == Event_CursorDrag)
		{
			// dragging a tab button
			if (args.capture->get_parent() == this)
			{
				if (args.capture->typespec()->identifier() == TYPESPEC_IDENTIFIER("gui::TabControl::TabButton"))
				{
					// Try to find a tab the cursor is located in.
					Point local = compositor_to_local(args.cursor);
					local.x = glm::clamp(local.x, 0.0f, static_cast<float>(size.width));
					for (size_t index = 0; index < tabs.size(); ++index)
					{
						TabButton* tab = tabs[index];
						const bool is_cursor_on_tab = ((local.x >= tab->get_origin().x) && (local.x <= (tab->get_origin().x + tab->get_size().width)));
						if (is_cursor_on_tab)
						{
							if (current_tab_index != index)
							{
								TabButton* temp = tabs[current_tab_index];
								tabs[current_tab_index] = tabs[index];
								tabs[index] = temp;
								current_tab_index = index;
								break;
							}
						}
					}
				}

				args.handled = 1;
				return;
			}
		}
	} // handle_event

	void TabControl::add_tab(size_t /*index*/, const std::string& name, Panel* panel)
	{
		current_tab = tabs.size();

		TabButton* new_tab = new TabButton(this, name);
		new_tab->set_panel(panel);
		//new_tab->on_click.bind<TabControl, &TabControl::tab_clicked>(this);

		// TODO: should get this from style
		new_tab->set_font("fonts/debug.ttf", 16);

		// TODO: handle vertical tabs?

		// TODO: Compute tab width based on name size.
		Rect text_bounds;
		get_compositor()->get_renderer()->font_measure_string(font, &name[0], name.size(), text_bounds);

		new_tab->set_origin(0, 0);
		new_tab->set_size(text_bounds.width(), tab_size.height);
		new_tab->set_name(name.c_str());
		new_tab->set_hover_color(gemini::Color(0, 1, 1));

		tabs.push_back(new_tab);

		// show the latest tab
		show_tab(current_tab);

		if (panel)
		{
			panel->parent = this;
			panel->set_origin(0, tab_size.height);
			panel->set_size(size.width, size.height - tab_size.height);
		}
	}

	void TabControl::remove_tab(size_t index)
	{
		TabButton* tab = tabs[index];
		Panel* panel = tab->get_panel();

		remove_child(panel);
		remove_child(tab);

		tabs.erase(tab);

		if (active_tab == tab)
		{
			// If you hit this, the last remaining tab was deleted.
			// Handle this gracefully.
			assert(!tabs.empty());

			// Need to restore another tab.
			show_tab(current_tab);
		}

		delete tab;
		delete panel;

	}

	void TabControl::show_tab(size_t index)
	{
		current_tab = index;

		TabButton* prev_tab = active_tab;
		if (prev_tab && prev_tab->get_panel())
		{
			prev_tab->get_panel()->set_visible(false);
		}
		active_tab = tabs[current_tab];

		if (active_tab && active_tab->get_panel())
		{
			active_tab->get_panel()->set_visible(true);
			active_tab->get_panel()->set_flags(active_tab->get_panel()->get_flags() | Flag_TransformIsDirty);
		}
	}

	void TabControl::update(Compositor* compositor, float delta_seconds)
	{
		Panel::update(compositor, delta_seconds);

		// Layout the tabs.
		const ScreenInt TAB_PADDING = 3;
		ScreenInt new_tab_x = 0;
		for (size_t index = 0; index < tabs.size(); ++index)
		{
			TabButton* tab = tabs[index];
			tab->set_origin(new_tab_x, 0);
			new_tab_x += static_cast<ScreenInt>(tab->get_size().width + TAB_PADDING);
		}

		if (active_tab && active_tab->get_panel())
		{
			active_tab->get_panel()->update(compositor, delta_seconds);
		}
	}

	void TabControl::render(Compositor* compositor, Renderer* renderer, render::CommandList& render_commands)
	{
		// draw the tab background
		render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], -1, gemini::Color::from_rgba(128, 0, 0, 255));

		render_children(compositor, renderer, render_commands);

		if (active_tab && active_tab->get_panel())
		{
			Panel* panel = active_tab->get_panel();
			panel->render(compositor, renderer, render_commands);
		}
	}
} // namespace gui

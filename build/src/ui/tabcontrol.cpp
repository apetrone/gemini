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

namespace gui
{
	TabControl::TabControl(Panel* root)
		: Panel(root)
		, active_tab(nullptr)
	{
		// TODO: should get this from style
		tab_size.width = 40;
		tab_size.height = 20;

		font = get_compositor()->get_resource_cache()->create_font("fonts/debug.ttf", 16);
		flags |= Flag_CanMove;
	}

	void TabControl::add_tab(size_t index, const std::string& name, Panel* panel)
	{
		current_tab = tabs.size();

		TabButton* new_tab = new TabButton(this, name);
		new_tab->set_panel(panel);
		new_tab->on_click.connect(&TabControl::tab_clicked, this);

		// TODO: should get this from style
		new_tab->set_font("fonts/debug.ttf", 16);

		// TODO: handle vertical tabs?
		new_tab->set_bounds(current_tab * (tab_size.width), 0, tab_size.width, tab_size.height);
		new_tab->set_name(name.c_str());
		new_tab->set_hover_color(gemini::Color(0, 1, 1));

		tabs.push_back(new_tab);

		// show the latest tab
		show_tab(current_tab);

		if (panel)
		{
			panel->parent = this;
			panel->set_origin(0, tab_size.height);
			panel->set_size(Size(size.width, size.height - tab_size.height));
		}
	}

	void TabControl::remove_tab(size_t index)
	{
		// TODO: implement this
		assert(0);
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
		}
	}

	void TabControl::update(Compositor* compositor, float delta_seconds)
	{
		Panel::update(compositor, delta_seconds);

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

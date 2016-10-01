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
#pragma once

#include <stdio.h>
#include <string>
#include <stdint.h>

#include <ui/button.h>

#include <core/typedefs.h>
#include <core/typespec.h>

namespace gui
{
	class TabControl : public Panel
	{
		TYPESPEC_DECLARE_CLASS(TabControl, Panel);

		class TabButton : public Button
		{
			TYPESPEC_DECLARE_CLASS(TabButton, Button);

		public:
			TabButton(Panel* parent, const std::string& name)
				: Button(parent)
				, panel(nullptr)
			{
				set_text(name);
				flags |= Flag_CanDrop | Flag_CanMove;
			}

			Panel* get_panel() const { return panel; }
			void set_panel(Panel* target) { panel = target; }

			virtual void handle_event(EventArgs& args) override;
			virtual void render(Compositor* compositor, Renderer* renderer, render::CommandList& render_commands) override;

		private:
			Panel* panel;
		};

		Array<TabButton*> tabs;
		size_t current_tab;
		TabButton* active_tab;

		size_t current_tab_index;

		Point last_position;
		Point tab_position;

		// the size of a tab's clickable region
		Size tab_size;

		FontHandle font;

		bool in_preview_mode;

	public:
		TabControl(Panel* root);

		virtual void update(Compositor* compositor, float delta_seconds) override;
		virtual void render(Compositor* compositor, Renderer* renderer, render::CommandList& render_commands) override;
		virtual void handle_event(EventArgs& args) override;
		void add_tab(size_t index, const std::string& name, Panel* panel);
		void remove_tab(size_t index);
		void show_tab(size_t index);

		//void tab_clicked(EventArgs& args);

		size_t get_active_tab_index() const { return current_tab+1; }
	};
} // namespace gui


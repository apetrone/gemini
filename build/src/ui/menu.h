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


#include <ui/button.h>

namespace gui
{
	class MenuButton : public gui::Button
	{
	public:
		MenuButton(gui::Panel* parent);
		virtual void handle_event(EventArgs& args) override;
	}; // MenuButton

	class Menu : public gui::Panel
	{
		gui::MenuButton* item;

		enum MenuItemType
		{
			MenuItem_Invalid,
			MenuItem_Item,
			MenuItem_Menu,
			MenuItem_Separator
		};

		struct MenuItem
		{
			MenuItemType type;
			const char* label;
			class Menu* menu;
			float height;
			Rect hit_rect;
			gemini::Delegate<void()> action;

			MenuItem()
				: type(MenuItem_Invalid)
				, label(nullptr)
				, menu(nullptr)
				, height(0.0)
			{
			}
		};

		Array<MenuItem> items;
		uint32_t menu_is_expanded;

		gui::Point menu_origin;
		FontHandle font_handle;
		int32_t font_height;

		Rect expanded_rect;

		void update_color();

	public:
		Menu(const char* label, Panel* parent);
		void add_item(const char* name, const gemini::Delegate<void()>& action);
		void add_menu(Menu* menu);
		void add_separator();

		void show();
		void hide();
		void toggle();
		bool is_open() const;

		gui::Button* get_button() { return item; }

		virtual void handle_event(EventArgs& args) override;
		virtual bool hit_test_local(const Point& local_point) const override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;

		const gui::Rect& get_expanded_rect() const { return expanded_rect; }
	}; // Menu


	class MenuBar : public Panel
	{
		float next_origin;
		Menu* last_menu;

		// is the menu bar currently showing an open menu?
		bool is_displaying_menu;

	public:
		MenuBar(Panel* parent);

		void add_menu(Menu* menu);
		virtual void handle_event(EventArgs& args) override;
		virtual bool hit_test_local(const Point& local_point) const override;
	}; // class MenuBar
} // namespace gui

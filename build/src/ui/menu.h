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
#include <core/typespec.h>

namespace gui
{
	class Menu;
	class MenuButton : public gui::Button
	{
		TYPESPEC_DECLARE_CLASS(MenuButton, Button);

	public:
		MenuButton(Panel* parent, Menu* menu);

		Menu* get_menu();

	private:
		Menu* menu;
	};


	enum MenuItemType
	{
		MenuItem_Invalid,
		MenuItem_Item,
		MenuItem_DropDown,
		MenuItem_Menu,
		MenuItem_Separator
	};

	class Menu : public gui::Panel
	{
		TYPESPEC_DECLARE_CLASS(Menu, Panel);

		MenuItemType item_type;

		gemini::Delegate<void()> action;

		std::string text;
		Point text_origin;
		FontHandle font_handle;
		int32_t font_height;

	public:
		Menu(const char* label, Panel* parent);
		void add_item(const char* name, const gemini::Delegate<void()>& action);
		void add_menu(Menu* menu);
		void add_separator();

		const char* get_text() const;

		void set_type(MenuItemType type);

		virtual void handle_event(EventArgs& args) override;
		virtual void update(Compositor* compositor, float delta_seconds) override;
		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
		virtual bool point_in_capture_rect(const Point&) const override { return true; }
	}; // Menu


	class MenuBar : public Panel
	{
		TYPESPEC_DECLARE_CLASS(MenuBar, Panel);

	public:
		MenuBar(Panel* parent);

		void add_menu(Menu* menu);

		Menu* find_menu_at_location(const Point& local_point);
		virtual void handle_event(EventArgs& args) override;
		virtual bool hit_test_local(const Point& local_point) const override;
		virtual bool point_in_capture_rect(const Point&) const override { return true; }
		virtual void update(Compositor* compositor, float delta_seconds) override;

	private:
		void update_size();

		float next_origin;

		// is the menu bar currently showing an open menu?
		bool is_displaying_menu;
	}; // class MenuBar
} // namespace gui

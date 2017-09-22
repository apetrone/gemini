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
#include <ui/compositor.h>
#include <ui/menu.h>

#include <core/logging.h>

TYPESPEC_REGISTER_CLASS(gui::Menu);
TYPESPEC_REGISTER_CLASS(gui::MenuButton);
TYPESPEC_REGISTER_CLASS(gui::MenuBar);

namespace gui
{
	const uint32_t MENU_ITEM_BORDER = 2;
	const uint32_t MENU_ITEM_SEPARATOR_HEIGHT = 2;

	// MenuButton

	MenuButton::MenuButton(Panel* parent, Menu* target_menu)
		: Button(parent)
	{
		menu = target_menu;
	}

	Menu* MenuButton::get_menu()
	{
		return menu;
	} // get_menu

	// Menu

	Menu::Menu(const char* label, Panel* parent)
		: Panel(parent)
	{
		flags |= Flag_CursorEnabled;
		set_name(label);

		font_handle = get_compositor()->get_resource_cache()->create_font("debug", 16);

		int ascender, descender;
		size_t height;
		get_compositor()->get_renderer()->font_metrics(font_handle, height, ascender, descender);
		font_height = static_cast<int32_t>(ascender + descender);

		Rect name_bounds;
		get_compositor()->get_renderer()->font_measure_string(font_handle, label, core::str::len(label), name_bounds);
		uint32_t dimx = static_cast<uint32_t>(name_bounds.width() + 8);
		set_size(dimx, static_cast<uint32_t>(parent->get_size().height));

		set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));

		text = label;
	}

	void Menu::add_item(const char* name, const gemini::Delegate<void()>& linked_action)
	{
		Menu* instance = new Menu(name, this);
		instance->item_type = MenuItem_Item;
		instance->action = linked_action;
		instance->set_size(40, 40);
	} // add_item

	void Menu::add_menu(Menu* /*menu*/)
	{
		// Implement sub menus!
		assert(0);
		//MenuItem menu_item;
		//menu_item.type = MenuItem_Menu;
		//menu_item.menu = menu;
		//items.push_back(menu_item);
	} // add_menu

	void Menu::add_separator()
	{
		Menu* instance = new Menu("", this);
		instance->item_type = MenuItem_Separator;
		instance->set_size(1, MENU_ITEM_SEPARATOR_HEIGHT);
		instance->set_background_color(gemini::Color(0.2f, 0.2f, 0.2f, 1.0f));
	} // add_separator

	const char* Menu::get_text() const
	{
		return text.c_str();
	}

	void Menu::set_type(MenuItemType type)
	{
		item_type = type;
	}

	void Menu::handle_event(EventArgs& args)
	{
		if (item_type == MenuItem_Item)
		{
			if (args.type == Event_CursorButtonPressed && args.target == this)
			{
				args.compositor->set_focus(this);
				args.handled = 1;
			}
			else if (args.type == Event_CursorButtonReleased && args.target == this)
			{
				action();
				args.handled = 1;
			}
		}
		else if (item_type == MenuItem_DropDown || item_type == MenuItem_Menu)
		{
			// Clicked on a menu item child of this menu -- so we hide this menu.
			if (args.type == Event_CursorButtonReleased && args.target->get_parent() == this)
			{
				set_visible(false);
			}
		}
	} // handle_event

	void Menu::update(Compositor* compositor, float delta_seconds)
	{
		Panel::update(compositor, delta_seconds);

		// local menu offset
		Point local_offset = Point(0, 0);

		// recompute the max menu size
		Size max_menu_size;

		// first pass, we need the maximum width of the menu items.
		for (size_t index = 0; index < total_children(); ++index)
		{
			Panel* child = children[index];
			Menu* menu = static_cast<Menu*>(child);

			if (menu->item_type == MenuItem_Item)
			{
				gui::Rect string_bounds;
				compositor->get_renderer()->font_measure_string(font_handle, menu->text.c_str(), core::str::len(menu->text.c_str()), string_bounds);
				menu->size.height = string_bounds.height() + 16; // add some padding to the height (+16)
				max_menu_size.width = glm::max(static_cast<uint32_t>(max_menu_size.width), string_bounds.width() + 16);
				max_menu_size.height += menu->size.height;
			}
			else if (menu->item_type == MenuItem_Separator)
			{
				max_menu_size.height += menu->size.height;
			}

			menu->set_origin(local_offset);

			local_offset.y += menu->size.height;
		}

		if (item_type == MenuItem_Item)
		{
			gui::Rect font_dims;
			compositor->get_renderer()->font_measure_string(font_handle, this->text.c_str(), text.size(), font_dims);

			// We need floor to snap to pixel boundaries; not fractional pixels;
			// which would introduce artifacts.

			// left-aligned
			text_origin.x = 6; // glm::floor((size.width / 2.0f) - (font_dims.width() / 2.0f));
			text_origin.y = glm::floor((size.height / 2.0f) - (font_dims.height() / 2.0f) + glm::max(static_cast<uint32_t>(font_height), font_dims.height()));
		}

		// Second pass, resize the items.
		for (size_t index = 0; index < total_children(); ++index)
		{
			Panel* child = children[index];
			Menu* menu = static_cast<Menu*>(child);

			menu->set_size(static_cast<uint32_t>(max_menu_size.width), static_cast<uint32_t>(menu->size.height));
		}

		// Separators *MAY* have zero width/height.
		// Don't hit the asserts in the compositor.
		if (item_type != MenuItem_Separator)
		{
			if (max_menu_size.width > 0 && max_menu_size.height > 0)
			{
				size = max_menu_size;
			}
		}
	} // update

	void Menu::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		Painter painter(this, render_commands);

		 // draw the background of the menu

		if (item_type != MenuItem_DropDown)
		{
			gemini::Color background_color(0.0f, 0.0f, 0.0f, 1.0f);

			if (item_type == MenuItem_Item && compositor->get_hot() == this)
			{
				// get this from style.
				background_color = gemini::Color(0.2f, 0.2f, 0.2f, 1.0f);
			}
			else if (item_type == MenuItem_Separator)
			{
				background_color = gemini::Color(0.3f, 0.3f, 0.3f, 1.0f);
			}


			Point data[4];
			data[0] = Point(0, 0);
			data[1] = Point(0, size.height);
			data[2] = Point(size.width, size.height);
			data[3] = Point(size.width, 0);

			painter.add_rectangle(data[0], data[1], data[2], data[3], -1, background_color);

			if (item_type == MenuItem_Item)
			{
				gui::Rect draw_bounds;
				draw_bounds.size = size;
				draw_bounds.origin = text_origin;

				if (!text.empty())
				{
					painter.add_font(font_handle, text.c_str(), text.size(), draw_bounds, foreground_color);
				}
			}
		}

		Panel::render_children(compositor, renderer, render_commands);
	} // render


	// MenuBar

	MenuBar::MenuBar(Panel* parent)
		: Panel(parent)
		, next_origin(0.0f)
		, is_displaying_menu(false)
	{
		flags |= Flag_CursorEnabled | Flag_AlwaysOnTop;
		set_name("MenuBar");

		set_origin(0, 0);
		update_size();
		set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));
	}


	void MenuBar::add_menu(Menu* menu)
	{
		MenuButton* button = new MenuButton(this, menu);
		button->set_name(menu->get_text());
		button->set_font("debug", 16);
		button->set_origin(next_origin, 0.0f);
		button->set_size(menu->get_size());
		button->set_text(menu->get_text());

		button->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		button->set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));
		button->set_hover_color(gemini::Color(0.2f, 0.2f, 0.2f, 1.0f));

		add_child(button);

		menu->set_type(MenuItem_DropDown);
		menu->set_origin(next_origin, size.height);
		menu->set_visible(false);
		add_child(menu);

		next_origin += menu->get_size().width;
	} // add_menu


	Menu* MenuBar::find_menu_at_location(const Point& local_point)
	{
		// Test other menu items...
		for (size_t index = 0; index < total_children(); ++index)
		{
			Menu* child = static_cast<Menu*>(child_at(index));
			Point test_point = local_point - child->get_origin();
			if (child->hit_test_local(test_point))
			{
				return child;
			}
		}

		return nullptr;
	} // find_menu_at_location


	void MenuBar::handle_event(EventArgs& args)
	{
		// Handle the user clicking on MenuButtons to toggle their menu's
		// visibility.
		const TypeSpecInfo* target_type = args.target->typespec();
		if (target_type->identifier() == TYPESPEC_IDENTIFIER("gui::MenuButton"))
		{
			MenuButton* menu_button = static_cast<MenuButton*>(args.target);
			Menu* menu = menu_button->get_menu();
			if (args.type == Event_CursorButtonPressed && (menu->item_type == MenuItem_DropDown))
			{
				if (menu->is_visible())
				{
					menu->set_visible(false);
					args.compositor->set_focus(nullptr);
					args.handled = 1;
				}
				else
				{
					menu->set_visible(true);
					args.compositor->set_focus(menu);
					args.handled = 1;
				}
			}

			// User is hovering over a different menu than is currently focused.
			if (args.focus && args.type == Event_CursorMove)
			{
				const TypeSpecInfo* focus_typespec = args.focus->typespec();
				if (focus_typespec->identifier() == TYPESPEC_IDENTIFIER("gui::Menu"))
				{
					Menu* focus_menu = static_cast<Menu*>(args.focus);
					if (focus_menu->is_visible() && (focus_menu != menu_button->get_menu()) && focus_menu->item_type == MenuItem_DropDown)
					{
						focus_menu->set_visible(false);
						menu_button->get_menu()->set_visible(true);
						args.compositor->set_focus(menu_button->get_menu());
						args.handled = 1;
					}
				}
			}
		}
		else if (target_type->identifier() == TYPESPEC_IDENTIFIER("gui::Menu") && args.type == Event_FocusLost)
		{
			// Focus changed from menu to another panel; hide panel.
			Menu* menu = static_cast<Menu*>(args.target);
			if (menu->item_type == MenuItem_DropDown)
			{
				// Don't hide a top level menu after we've clicked an item on that menu.
				if (args.capture && args.capture->get_parent() != args.target)
				{
					menu->set_visible(false);
					args.handled = 1;
				}
			}
		}

		if (args.type == Event_CursorButtonPressed && args.target == this)
		{
			args.compositor->set_focus(this);
			args.handled = 1;
		}

		Panel::handle_event(args);
	} // handle_event

	bool MenuBar::hit_test_local(const Point& local_point) const
	{
		bool hit_panel = Panel::hit_test_local(local_point);
		if (!hit_panel)
		{
			// Test other menu items if the panel hit failed...
			for (size_t index = 0; index < total_children(); ++index)
			{
				Panel* child = child_at(index);
				Point test_point = local_point - child->get_origin();
				if (child->has_flags(Flag_CursorEnabled | Flag_IsVisible) && child->hit_test_local(test_point))
				{
					return true;
				}
			}
		}

		return hit_panel;
	} // hit_test_local

	void MenuBar::update(Compositor* compositor, float delta_seconds)
	{
		update_size();

		Panel::update(compositor, delta_seconds);
	} // update

	void MenuBar::update_size()
	{
		const float parent_width = parent->get_size().get_width();
		set_size(gui::Size(parent_width, 24));
		set_maximum_size(gui::Size(parent_width, 24));
	} // update_size
} // namespace gui

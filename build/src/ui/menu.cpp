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

namespace gui
{
	// MenuButton

	MenuButton::MenuButton(gui::Panel* parent)
		: gui::Button(parent)
	{
	}

	void MenuButton::handle_event(EventArgs& args)
	{
		// events that propagate from this should point to the parent
		// (menu) as the target.
		args.target = parent;
		Panel::handle_event(args);
	}


	// Menu

	void Menu::update_color()
	{
		if (menu_is_expanded)
		{
			set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 1.0f));
			item->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 1.0f));
			item->set_hover_color(gemini::Color(0.0f, 0.0f, 0.0f, 1.0f));
		}
		else
		{
			set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));
			item->set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));
			item->set_hover_color(gemini::Color(0.2f, 0.2f, 0.2f, 1.0f));
		}
	} // update_color

	Menu::Menu(const char* label, Panel* parent)
		: Panel(parent)
		, menu_is_expanded(0)
	{
		flags |= Flag_CursorEnabled;
		set_name(label);
		item = new gui::MenuButton(this);

		set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));

		item->set_font("fonts/debug.ttf", 16);
		item->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		item->set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));
		item->set_hover_color(gemini::Color(0.2f, 0.2f, 0.2f, 1.0f));
		item->set_origin(0, 0);
		item->set_dimensions(1.0f, 1.0f);
		item->set_text(label);

		font_handle = get_compositor()->get_resource_cache()->create_font("fonts/debug.ttf", 16);

		int ascender, descender;
		size_t height;
		get_compositor()->get_renderer()->font_metrics(font_handle, height, ascender, descender);
		font_height = static_cast<int32_t>(ascender + descender);

		Rect name_bounds;
		get_compositor()->get_renderer()->font_measure_string(font_handle, label, core::str::len(label), name_bounds);
		float dimx = (name_bounds.width() + 8.0) / parent->get_size().width;
		set_dimensions(dimx, 1.0f);
	}

	void Menu::add_item(const char* name, const gemini::Delegate<void()>& action)
	{
		MenuItem item;
		item.type = MenuItem_Item;
		item.label = name;
		item.action = action;
		items.push_back(item);
	} // add_item

	void Menu::add_menu(Menu* menu)
	{
		MenuItem item;
		item.type = MenuItem_Menu;
		item.menu = menu;
		items.push_back(item);
	} // add_menu

	void Menu::add_separator()
	{
		MenuItem item;
		item.type = MenuItem_Separator;
		item.height = MENU_ITEM_SEPARATOR_HEIGHT;
		items.push_back(item);
	} // add_separator

	void Menu::show()
	{
		menu_is_expanded = 1;
		update_color();
	} // show

	void Menu::hide()
	{
		menu_is_expanded = 0;
		update_color();
	} // hide

	void Menu::toggle()
	{
		menu_is_expanded = !menu_is_expanded;
		update_color();
	} // toggle

	bool Menu::is_open() const
	{
		return menu_is_expanded;
	} // is_open

	void Menu::handle_event(EventArgs& args)
	{
		if (args.type == Event_FocusGain)
		{
			show();
		}
		else if (args.type == Event_FocusLost)
		{
			hide();
		}
		else if (args.type == Event_CursorButtonReleased)
		{
			MenuItem* found_item = nullptr;
			Point local_cursor = compositor_to_local(args.cursor) - Point(0, size.height);
			Rect test_rect;
			for (MenuItem& item : items)
			{
				if (item.hit_rect.is_point_inside(local_cursor))
				{
					found_item = &item;
					break;
				}
			}

			if (found_item)
			{
				get_compositor()->set_focus(nullptr);

				assert(found_item->action.is_valid());
				found_item->action();

				args.handled = true;
			}
		}

		// don't handle anything; just pass this on.
		Panel::handle_event(args);
	} // handle_event

	bool Menu::hit_test_local(const Point& local_point) const
	{
		if (menu_is_expanded)
		{
			if (expanded_rect.is_point_inside(local_point))
			{
				return true;
			}
		}

		return Panel::hit_test_local(local_point);
	} // hit_test_local

	void Menu::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		const gemini::Color highlight_color(0.2f, 0.2f, 0.2f, 1.0f);

		if (menu_is_expanded)
		{
			// local menu offset
			Point local_offset = Point(0, size.height);//  compositor_to_local(menu_origin);
			const Point padding(8, 16);

			// first pass, we need the maximum width of the menu items.
			Size max_menu_size;
			for (MenuItem& item : items)
			{
				if (item.type == MenuItem_Item)
				{
					gui::Rect string_bounds;
					compositor->get_renderer()->font_measure_string(font_handle, item.label, core::str::len(item.label), string_bounds);
					max_menu_size.width = glm::max(max_menu_size.width, string_bounds.width() + padding.x + padding.x);
					max_menu_size.height += item.height = string_bounds.height() + 16; // add some padding to the height
				}
				else if (item.type == MenuItem_Separator)
				{
					max_menu_size.height += item.height;
				}
			}

			expanded_rect = Rect(local_offset, max_menu_size);

			// draw the background of the menu
			{

				Point data[4];
				data[0] = local_offset;
				data[1] = local_offset + Point(0, max_menu_size.height);
				data[2] = local_offset + Point(max_menu_size.width, max_menu_size.height);
				data[3] = local_offset + Point(max_menu_size.width, 0);

				render_commands.add_rectangle(
					transform_point(get_transform(0), data[0]),
					transform_point(get_transform(0), data[1]),
					transform_point(get_transform(0), data[2]),
					transform_point(get_transform(0), data[3]),
					-1,
					gemini::Color(0.0f, 0.0f, 0.0f, 1.0f)
					);
			}

			// calculate the local cursor in this menu's space
			const Point& composite_cursor = compositor->get_cursor_position();

			// we have to factor in the menu_origin in local space.
			Point local_cursor = compositor_to_local(composite_cursor) - local_offset;

			expanded_rect = Rect(local_offset, max_menu_size);
			float current_item_offset = 0;
			size_t index = 0;
			for (MenuItem& item : items)
			{
				if (item.type == MenuItem_Item)
				{
					Size menu_size;
					menu_size.width = max_menu_size.width;
					menu_size.height = item.height;

					Point data[4];
					data[0] = local_offset + Point(MENU_ITEM_BORDER, MENU_ITEM_BORDER);
					data[1] = local_offset + Point(MENU_ITEM_BORDER, menu_size.height - MENU_ITEM_BORDER);
					data[2] = local_offset + Point(menu_size.width - MENU_ITEM_BORDER, menu_size.height - MENU_ITEM_BORDER);
					data[3] = local_offset + Point(menu_size.width - MENU_ITEM_BORDER, MENU_ITEM_BORDER);

					item.hit_rect.origin.y = current_item_offset;
					item.hit_rect.size = menu_size;


					gemini::Color draw_color = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
					gemini::Color font_color = gemini::Color(1.0f, 1.0f, 1.0f);
					if (item.hit_rect.is_point_inside(local_cursor))
					{
						draw_color = highlight_color;
					}

					render_commands.add_rectangle(
						transform_point(get_transform(0), data[0]),
						transform_point(get_transform(0), data[1]),
						transform_point(get_transform(0), data[2]),
						transform_point(get_transform(0), data[3]),
						-1,
						draw_color
						);

					if (item.label)
					{
						Rect font_rect;
						font_rect.origin = transform_point(get_transform(0), local_offset);
						font_rect.size = menu_size;
						font_rect.origin.x += 8;
						font_rect.origin.y += font_height + 8;
						render_commands.add_font(font_handle, item.label, core::str::len(item.label), font_rect, font_color);
					}

					current_item_offset += item.height;
					local_offset.y += item.height;
				}
				else if (item.type == MenuItem_Separator)
				{
					Point start(0, local_offset.y);
					Point end(max_menu_size.width, local_offset.y);
					render_commands.add_line(
						transform_point(get_transform(0), start),
						transform_point(get_transform(0), end), highlight_color);
					current_item_offset += item.height;
					local_offset.y += item.height;
				}

				index++;
			}
		}

		Panel::render(compositor, renderer, render_commands);
	} // render


	// MenuBar

	MenuBar::MenuBar(Panel* parent)
		: Panel(parent)
		, next_origin(4.0f)
		, last_menu(nullptr)
		, is_displaying_menu(false)
	{
		flags |= Flag_CursorEnabled | Flag_AlwaysOnTop;
		set_name("MenuBar");
	}

	void MenuBar::add_menu(Menu* menu)
	{
		const Point& dimensions = menu->get_dimensions();

		menu->set_origin(next_origin, 0.0f);
		next_origin += (dimensions.x * size.width);
	} // add_menu

	void MenuBar::handle_event(EventArgs& args)
	{
		if (args.target != this)
		{
			Menu* menu = static_cast<Menu*>(args.target);
			if (args.type == Event_CursorButtonPressed)
			{
				// Pressing a top-level menu item should
				// toggle the menu show/hide.
				menu->toggle();
				last_menu = menu;
			}
			else if (args.type == Event_CursorEnter)
			{
				// When the cursor hovers over an expanded menu item,
				// it must switch focus to that menu.
				if (last_menu && (last_menu != menu) && last_menu->is_open())
				{
					get_compositor()->set_focus(menu);
				}

				last_menu = menu;
			}
		}

		args.handled = 1;
	} // handle_event

	bool MenuBar::hit_test_local(const Point& local_point) const
	{
		if (last_menu && last_menu->is_open())
		{
			gui::Rect expanded_rect = last_menu->get_expanded_rect();
			expanded_rect.origin += last_menu->get_origin();
			if (expanded_rect.is_point_inside(local_point))
			{
				return true;
			}
		}

		return Panel::hit_test_local(local_point);
	} // hit_test_local

} // namespace gui

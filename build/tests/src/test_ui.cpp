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
#include "unit_test.h"

#include <core/logging.h>

#include <runtime/filesystem.h>
#include <runtime/runtime.h>
#include <runtime/guirenderer.h>
#include <runtime/standaloneresourcecache.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <renderer/renderer.h>
#include <renderer/vertexbuffer.h>
#include <renderer/vertexstream.h>
#include <renderer/font.h>

#include <ui/ui.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/button.h>
#include <ui/slider.h>
#include <ui/label.h>
#include <ui/tabcontrol.h>

#include <core/threadsafequeue.h>


#include <assert.h>

using namespace renderer;

// The simplest of all tests -- see what happens when we create a new panel
// and put it on the compositor.
#define GEMINI_TEST_PANEL			1
#define GEMINI_TEST_GRAPH			1
#define GEMINI_TEST_MENU			1
#define GEMINI_TEST_TABCONTROL		1
#define GEMINI_TEST_BUTTON			1
#define GEMINI_TEST_SLIDER			1

// investigate two pass method for layouts:
// measure + arrange
// http://www.gamedev.net/topic/673813-gui-elements-whose-positions-depend-on-one-another/?hl=%2Bgui+%2Blayout#entry5266332
// MSDN's documentation on the WPF layout system

// ---------------------------------------------------------------------
// gui
// ---------------------------------------------------------------------
static void* gui_malloc_callback(size_t size)
{
	return core::memory::global_allocator().allocate(size, sizeof(void*), __FILE__, __LINE__);
}

static void gui_free_callback(void* pointer)
{
	core::memory::global_allocator().deallocate(pointer);
}

struct MyVertex
{
	float position[3];
	float color[4];

	void set_position(float x, float y, float z)
	{
		position[0] = x;
		position[1] = y;
		position[2] = z;
	}

	void set_color(float red, float green, float blue, float alpha)
	{
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		color[3] = alpha;
	}
};


namespace gui
{
	class MenuButton : public gui::Button
	{
	public:

		MenuButton(gui::Panel* parent)
			: gui::Button(parent)
		{
		}

		virtual void handle_event(EventArgs& args) override
		{
			// events that propagate from this should point to the parent
			// (menu) as the target.
			args.target = parent;
			Panel::handle_event(args);
		}
	};

	// This uses a render target to present data
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

		static const uint32_t MENU_ITEM_BORDER = 2;
		static const uint32_t MENU_ITEM_SEPARATOR_HEIGHT = 1;

	public:
		Menu(const char* label, Panel* parent)
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

		void add_item(const char* name, const gemini::Delegate<void()>& action)
		{
			MenuItem item;
			item.type = MenuItem_Item;
			item.label = name;
			item.action = action;
			items.push_back(item);
		}

		void add_menu(Menu* menu)
		{
			MenuItem item;
			item.type = MenuItem_Menu;
			item.menu = menu;
			items.push_back(item);
		}

		void add_separator()
		{
			MenuItem item;
			item.type = MenuItem_Separator;
			item.height = MENU_ITEM_SEPARATOR_HEIGHT;
			items.push_back(item);
		}

		void update_color()
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
		}

		void show()
		{
			menu_is_expanded = 1;
			update_color();
		}

		void hide()
		{
			menu_is_expanded = 0;
			update_color();
		}

		void toggle()
		{
			menu_is_expanded = !menu_is_expanded;
			update_color();
		}

		bool is_open() const
		{
			return menu_is_expanded;
		}

		gui::Button* get_button() { return item; }

		virtual void handle_event(EventArgs& args) override
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
		}

		virtual bool hit_test_local(const Point& local_point) const override
		{
			if (menu_is_expanded)
			{
				if (expanded_rect.is_point_inside(local_point))
				{
					return true;
				}
			}

			return Panel::hit_test_local(local_point);
		}

		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
		{
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
						max_menu_size.width = glm::max(max_menu_size.width, string_bounds.width()+padding.x+padding.x);
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
						data[1] = local_offset + Point(MENU_ITEM_BORDER, menu_size.height-MENU_ITEM_BORDER);
						data[2] = local_offset + Point(menu_size.width-MENU_ITEM_BORDER, menu_size.height-MENU_ITEM_BORDER);
						data[3] = local_offset + Point(menu_size.width-MENU_ITEM_BORDER, MENU_ITEM_BORDER);

						item.hit_rect.origin.y = current_item_offset;
						item.hit_rect.size = menu_size;

						gemini::Color draw_color = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
						gemini::Color font_color = gemini::Color(1.0f, 1.0f, 1.0f);
						if (item.hit_rect.is_point_inside(local_cursor))
						{
							draw_color = gemini::Color(0.2f, 0.2f, 0.2f, 1.0f);
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
							font_rect.origin.y += font_height+8;
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
							transform_point(get_transform(0), end), gemini::Color(0.2f, 0.2f, 0.2f));
						current_item_offset += item.height;
						local_offset.y += item.height;
					}

					index++;
				}
			}

			Panel::render(compositor, renderer, render_commands);
		}

		const gui::Rect& get_expanded_rect() const { return expanded_rect; }
	}; // Menu



	class MenuBar : public Panel
	{
		float next_origin;
		Menu* last_menu;

		// is the menu bar currently showing an open menu?
		bool is_displaying_menu;

	public:
		MenuBar(Panel* parent)
			: Panel(parent)
			, next_origin(4.0f)
			, last_menu(nullptr)
			, is_displaying_menu(false)
		{
			flags |= Flag_CursorEnabled | Flag_AlwaysOnTop;
			set_name("MenuBar");
		}

		void add_menu(Menu* menu)
		{
			const Point& dimensions = menu->get_dimensions();

			menu->set_origin(next_origin, 0.0f);
			next_origin += (dimensions.x * size.width);
		}

		virtual void handle_event(EventArgs& args) override
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
		}

		virtual bool hit_test_local(const Point& local_point) const override
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
		}
	}; // class MenuBar
}



// ---------------------------------------------------------------------
// TestUi
// ---------------------------------------------------------------------
class TestUi : public kernel::IKernel,
	public kernel::IEventListener<kernel::KeyboardEvent>,
	public kernel::IEventListener<kernel::MouseEvent>,
	public kernel::IEventListener<kernel::SystemEvent>
{
	bool active;
	platform::window::NativeWindow* native_window;
	gui::Compositor* compositor;
	gui::Graph* graph;
	gui::Label* label;
	gui::Slider* slider;
	gui::Label* slider_label;
	GUIRenderer renderer;
	StandaloneResourceCache resource_cache;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;

public:
	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.is_down)
		{
			if (event.key == input::BUTTON_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
		}
	}

	virtual void event(kernel::MouseEvent& event)
	{
		if (event.subtype == kernel::MouseMoved)
		{
			compositor->cursor_move_absolute(event.mx, event.my);
		}
		else if (event.subtype == kernel::MouseButton)
		{
			gui::CursorButton::Type input_to_gui[] = {
				gui::CursorButton::None,
				gui::CursorButton::Left,
				gui::CursorButton::Right,
				gui::CursorButton::Middle,
				gui::CursorButton::Mouse4,
				gui::CursorButton::Mouse5
			};

			if (event.is_down)
			{
				platform::window::set_mouse_tracking(true);
			}
			else
			{
				platform::window::set_mouse_tracking(false);
			}
			compositor->cursor_button(input_to_gui[event.button], event.is_down);
		}
	}

	virtual void event(kernel::SystemEvent& event)
	{
		switch(event.subtype)
		{
			case kernel::WindowResized:
			{
				LOGV("window resized: %i x %i\n", event.render_width, event.render_height);
				if (device)
				{
					device->backbuffer_resized(event.render_width, event.render_height);
				}

				compositor->resize(event.render_width, event.render_height);
				break;
			}
			case kernel::WindowClosed:
			{
				LOGV("Window was closed!\n");
				set_active(false);
				break;
			}

			default:
				break;
		}
	}

public:

	TestUi() :
		renderer(resource_cache)
	{
		native_window = nullptr;
		active = true;
		graph = nullptr;
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	void slider_value_changed(float new_value)
	{
		if (slider_label)
		{
			slider_label->set_text(
				core::str::format("%2.2f", new_value)
			);
		}
	}

	void test_button_clicked(gui::EventArgs& args)
	{
		fprintf(stdout, "test_button_clicked\n");
	}

	void on_new_project()
	{
	}

	void on_open_project(void)
	{
	}

	void on_quit(void)
	{
		set_active(false);
	}


	void on_package()
	{
		LOGV("package called\n");
	}

	void on_package_and_run()
	{
		LOGV("package and run\n");
	}

	void setup_gui(int width, int height)
	{
		graph = nullptr;
		label = nullptr;
		slider = nullptr;
		renderer.set_device(device);

		gui::set_allocator(gui_malloc_callback, gui_free_callback);
		compositor = new gui::Compositor(width, height, &resource_cache, &renderer);
		compositor->set_name("main_compositor");

		platform::window::Frame frame = platform::window::get_render_frame(native_window);

		// Window frame is invalid!
		assert(frame.width > 0);
		assert(frame.height > 0);

		const char dev_font[] = "fonts/debug.ttf";
		const char menu_font[] = "fonts/debug.ttf";

		const size_t dev_font_size = 16;
		const size_t menu_font_size = 48;


		// test the menu bar
#if GEMINI_TEST_MENU
		gui::MenuBar* menubar = new gui::MenuBar(compositor);
		menubar->set_origin(0, 0);
		menubar->set_size(gui::Size(frame.width, 24));
		menubar->set_maximum_size(gui::Size(frame.width, 24));
		menubar->set_background_color(gemini::Color(0.15f, 0.15f, 0.15f, 1.0f));

		gui::Menu* filemenu = new gui::Menu("File", menubar);
		filemenu->add_item("New Project", MAKE_MEMBER_DELEGATE(void(), TestUi, &TestUi::on_new_project, this));
		filemenu->add_item("Open Project...", MAKE_MEMBER_DELEGATE(void(), TestUi, &TestUi::on_open_project, this));
		filemenu->add_separator();
		filemenu->add_item("Quit", MAKE_MEMBER_DELEGATE(void(), TestUi, &TestUi::on_quit, this));
		menubar->add_menu(filemenu);

		gui::Menu* deploymenu = new gui::Menu("Deploy", menubar);
		deploymenu->add_item("Package...", MAKE_MEMBER_DELEGATE(void(), TestUi, &TestUi::on_package, this));
		deploymenu->add_separator();
		deploymenu->add_item("Package and Run", MAKE_MEMBER_DELEGATE(void(), TestUi, &TestUi::on_package_and_run, this));
		menubar->add_menu(deploymenu);

		gui::Menu* helpmenu = new gui::Menu("Help", menubar);
		helpmenu->add_item("About...", gemini::Delegate<void()>());
		menubar->add_menu(helpmenu);
#endif

#if GEMINI_TEST_PANEL
		gui::HBoxLayout* panel_layout = new gui::HBoxLayout();

		gui::Panel* test_panel = new gui::Panel(compositor);
		test_panel->set_origin(0, 24);
		test_panel->set_maximum_size(gui::Size(400, 400));
		//test_panel->set_rotation(mathlib::degrees_to_radians(-15));
		test_panel->set_background_color(gemini::Color(0.5f, 0.0f, 0.5f, 1.0f));
		test_panel->set_layout(panel_layout);

		const size_t TOTAL_BUTTONS = 32;
		const float COLOR_INCREMENT = (1.0f / TOTAL_BUTTONS);
		for (size_t index = 0; index < TOTAL_BUTTONS; ++index)
		{
			gui::Button* button0 = new gui::Button(test_panel);
			const float inc = COLOR_INCREMENT * index;
			button0->set_background_color(gemini::Color(inc, 0.5f + (inc < 0.5) ? inc : -0.5f, (1.0f - (COLOR_INCREMENT*index))));
			button0->set_font("fonts/debug.ttf", 16);
			button0->set_text(core::str::format("button %i", index));
			button0->set_name("button");
		}
#endif

		// setup the frame rate graph
#if GEMINI_TEST_GRAPH
		graph = new gui::Graph(compositor);
		graph->set_origin(width - 250, 24);
		graph->set_dimensions(graph->dimensions_from_pixels(gui::Point(250, 100)));
		graph->set_maximum_size(gui::Size(250, 100));
		graph->set_font(dev_font, dev_font_size);
		graph->set_background_color(gemini::Color::from_rgba(60, 60, 60, 255));
		graph->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gemini::Color::from_rgba(0, 255, 0, 255));
		graph->set_range(0.0f, 33.3f);
		graph->enable_baseline(true, 16.6f, gemini::Color::from_rgba(255, 0, 255, 255));
#endif

		// test tab panel
#if GEMINI_TEST_TABCONTROL
		gui::TabControl* tab = new gui::TabControl(compositor);
		tab->set_origin(425, 30);
		tab->set_dimensions(tab->dimensions_from_pixels(gui::Point(250, 250)));
		tab->set_name("tab_panel");

		label = new gui::Label(tab);
		label->set_origin(50, 115);
		label->set_dimensions(label->dimensions_from_pixels(gui::Point(110, 40)));
		label->set_background_color(gemini::Color::from_rgba(32, 32, 32, 255));
		label->set_foreground_color(gemini::Color::from_rgba(0, 255, 0, 255));
		label->set_font(dev_font, dev_font_size);
		const char str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
		const size_t MAX_LABEL_LINES = 26;
		for (size_t index = 0; index < MAX_LABEL_LINES; ++index)
		{
			label->append_text(str);
		}

		tab->add_tab(0, "test", label);

		{
			label = new gui::Label(tab);
			label->set_origin(50, 115);
			label->set_dimensions(label->dimensions_from_pixels(gui::Point(110, 40)));
			label->set_background_color(gemini::Color::from_rgba(32, 32, 32, 255));
			label->set_foreground_color(gemini::Color::from_rgba(255, 0, 0, 255));
			label->set_font(dev_font, dev_font_size);
			label->set_text("adam 0123456789");
			tab->add_tab(1, "test2", label);
		}
#endif
		// test buttons
#if GEMINI_TEST_BUTTON
		gemini::Color button_background = gemini::Color::from_rgba(128, 128, 128, 255);
		gemini::Color button_hover = gemini::Color::from_rgba(255, 255, 128, 255);

		uint32_t button_width = 320;
		uint32_t button_height = 50;
		uint32_t button_spacing = 10;
		const size_t total_buttons = 4;
//		uint32_t vertical_offset = 0;
		uint32_t origin_x = (compositor->get_size().width/2.0f) - (button_width/2.0f);
		uint32_t origin_y = (compositor->get_size().height/2.0f) - ((button_height*total_buttons)/2.0f);

		const char* captions[total_buttons] = {
			"New Game",
			"Test Button",
			"Test Two",
			"Test Three"
		};

		gui::Button* buttons[total_buttons] = {nullptr};

		for (size_t index = 0; index < total_buttons; ++index)
		{
			gui::Button* button = new gui::Button(compositor);
			button->set_origin(origin_x, origin_y);
			button->set_dimensions(button->dimensions_from_pixels(gui::Point(button_width, button_height)));
			button->set_font(menu_font, menu_font_size);
			button->set_text(captions[index]);
			button->set_background_color(button_background);
			button->set_hover_color(button_hover);
			button->on_click.bind<TestUi, &TestUi::test_button_clicked>(this);

			origin_y += button_height + button_spacing;
			buttons[index] = button;
		}


		buttons[0]->set_visible(false);
		LOGV("button %p is not visible\n", buttons[0]);
#endif

		// test slider

#if GEMINI_TEST_SLIDER
		// slider label to check value
		slider_label = new gui::Label(compositor);
		slider_label->set_origin(230, 450);
		slider_label->set_dimensions(slider_label->dimensions_from_pixels(gui::Point(40, 30)));
		slider_label->set_background_color(gemini::Color::from_rgba(0, 0, 0, 0));
		slider_label->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
		slider_label->set_text("empty");
		slider_label->set_font("fonts/debug.ttf", 16);

		gui::Size slider_size(200, 30);
		slider = new gui::Slider(compositor);
		slider->set_origin(20, 450);
		slider->set_dimensions(slider->dimensions_from_pixels(gui::Point(200, 40)));
		slider->set_background_color(gemini::Color::from_rgba(60, 60, 60, 255));
		slider->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
		slider->on_value_changed.bind<TestUi, &TestUi::slider_value_changed>(this);
		slider->set_value(0.5f);
#endif

	}

	virtual kernel::Error startup()
	{
		gemini::runtime_startup("arcfusion.net/gemini/test_ui");

		input::startup();

		platform::window::startup(platform::window::RenderingBackend_Default);

		size_t total_displays = platform::window::screen_count();
		LOGV("-> total screens: %lu\n", total_displays);

		for (size_t i = 0; i < total_displays; ++i)
		{
			platform::window::Frame frame = platform::window::screen_frame(i);
			LOGV("display %lu rect = %2.2f, %2.2f, %2.2f x %2.2f\n", (unsigned long)i, frame.x, frame.y, frame.width, frame.height);
		}



		// create a test window
		platform::window::Parameters params;
		params.frame = platform::window::centered_window_frame(0, 1280, 720);
		params.window_title = "test_ui";

		native_window = platform::window::create(params);
		assert(native_window != nullptr);
		platform::window::Frame window_frame = platform::window::get_frame(native_window);
		LOGV("window dimensions: %2.2f %2.2f\n", window_frame.width, window_frame.height);

		platform::window::focus(native_window);

		// initialize render device
		render2::RenderParameters render_parameters;
		render_parameters["rendering_backend"] = "default";
		render_parameters["gamma_correct"] = "true";

		device = render2::create_device(render_parameters);
		assert(device != nullptr);

		window_frame = platform::window::get_frame(native_window);

		device->init(static_cast<int>(window_frame.width), static_cast<int>(window_frame.height));

		// setup the pipeline
		render2::PipelineDescriptor desc;
		desc.shader = device->create_shader("vertexcolor");
		desc.vertex_description.add("in_position", render2::VD_FLOAT, 3); // position
		desc.vertex_description.add("in_color", render2::VD_FLOAT, 4); // color
		desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
		pipeline = device->create_pipeline(desc);

		// create a vertex buffer and populate it with data
		float width = (float)window_frame.width;
		float height = (float)window_frame.height;

		// Draw a triangle on screen with the wide part of the base at the bottom
		// of the screen.
		size_t total_bytes = sizeof(MyVertex) * 4;
		vertex_buffer = device->create_vertex_buffer(total_bytes);
		assert(vertex_buffer != nullptr);
		MyVertex vertices[4];
		vertices[0].set_position(0, height, 0);
		vertices[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);

		vertices[1].set_position(width, height, 0);
		vertices[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);

		vertices[2].set_position(width/2, 0, 0);
		vertices[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);

		vertices[3].set_position(0, 0, 0);
		vertices[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

		device->buffer_upload(vertex_buffer, vertices, total_bytes);


		// setup constant buffer
		modelview_matrix = glm::mat4(1.0f);
		projection_matrix = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);

		pipeline->constants().set("modelview_matrix", &modelview_matrix);
		pipeline->constants().set("projection_matrix", &projection_matrix);

		kernel::parameters().step_interval_seconds = (1.0f/50.0f);


		// initialize fonts
		font::startup(device);

		// initialize gui
		setup_gui(window_frame.width, window_frame.height);

		return kernel::NoError;
	}

	void update()
	{
		assert(kernel::parameters().step_interval_seconds != 0.0f);
		uint64_t current_time = platform::microseconds();
		kernel::Parameters& params = kernel::parameters();

		// need to initialize this to current_time otherwise the initial
		// delta could be very large; which leads to an infinite loop
		// when processing the accumulator.
		static uint64_t last_time = current_time;
		static float accumulator = 0;

		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time)*0.001f;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds*0.001f;

		last_time = current_time;

		// update accumulator
		accumulator += params.framedelta_seconds;

		while(accumulator >= params.step_interval_seconds)
		{
			// subtract the interval from the accumulator
			accumulator -= static_cast<float>(params.step_interval_seconds);

			// increment tick counter
			params.current_tick++;
		}

		params.step_alpha = static_cast<float>(accumulator / params.step_interval_seconds);
		if ( params.step_alpha >= 1.0f )
		{
			params.step_alpha -= 1.0f;
		}
	}

	virtual void tick()
	{
		update();

		// update our input
		input::update();

		// dispatch all window events
		platform::window::dispatch_events();

		if (graph)
		{
			graph->record_value(kernel::parameters().framedelta_milliseconds, 0);
		}

		// sanity check
		assert(device);
		assert(pipeline);
		assert(vertex_buffer);

		static float rot = 0.0f;

		rot += 10.f*kernel::parameters().framedelta_seconds;

		if (label)
		{
			core::memory::Zone* zone = core::memory::global_allocator().memory_zone;
			label->set_text(core::str::format("total_bytes: %i, total_allocations: %i", zone->get_total_bytes(), zone->get_total_allocations()));
		}


//		graph->set_rotation(mathlib::degrees_to_radians(rot));

		if (rot > 360)
			rot -= 360.0f;

		// update the gui
		compositor->tick(kernel::parameters().framedelta_seconds);

#if 1
		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		assert(render_pass.target->width != 0 && render_pass.target->height != 0);
		render_pass.color(0.0f, 0.0f, 0.0f, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		// create a command queue
		render2::CommandQueue* queue = device->create_queue(render_pass);

		// create a command serializer for the queue
		render2::CommandSerializer* serializer = device->create_serializer(queue);
		assert(serializer);

		// add commands to the queue
		serializer->pipeline(pipeline);
//		serializer->viewport(0, 0, native_window->dimensions.width, native_window->dimensions.height);
		serializer->vertex_buffer(vertex_buffer);
//		serializer->draw_indexed_primitives(index_buffer, 3);
		serializer->draw(0, 3);
		device->destroy_serializer(serializer);

		// queue the buffer with our device
		device->queue_buffers(queue, 1);


		compositor->draw();
#endif

		platform::window::activate_context(native_window);
		device->submit();
		platform::window::swap_buffers(native_window);
	}


	virtual void shutdown()
	{
		// shutdown/destroy the gui
		delete compositor;

		resource_cache.clear();

		// shutdown the fonts
		font::shutdown();

		device->destroy_buffer(vertex_buffer);
		device->destroy_pipeline(pipeline);
		render2::destroy_device(device);

		platform::window::destroy(native_window);
		platform::window::shutdown();

		input::shutdown();

		gemini::runtime_shutdown();
	}


private:
	render2::Device* device;
	render2::Pipeline* pipeline;
	render2::Buffer* vertex_buffer;
};

// Rule of three governs if you implement any ONE of the following,
// then you should implement ALL  three.
// - destructor
// - copy constructor
// - copy assignment operator

// The rule of five (C++11) governs that if you implement any ONE of the following,
// you should implement ALL five.
// - destructor
// - copy constructor
// - copy assignment operator
// - move constructor
// - move assignment operator


PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new TestUi()));
}

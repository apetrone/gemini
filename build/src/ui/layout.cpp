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
//#include "ui/ui.h"
#include <ui/panel.h>
#include <ui/layout.h>

//#include <stdlib.h>
TYPESPEC_REGISTER_CLASS(gui::Layout);
TYPESPEC_REGISTER_CLASS(gui::Spacer);
TYPESPEC_REGISTER_CLASS(gui::BoxLayout);
TYPESPEC_REGISTER_CLASS(gui::HorizontalLayout);
TYPESPEC_REGISTER_CLASS(gui::VerticalLayout);
TYPESPEC_REGISTER_CLASS(gui::DockingLayout);

namespace gui
{
	Layout::Layout()
	{
	}

	Layout::~Layout()
	{
	}

	void* Layout::operator new(size_t bytes)
	{
		return _gmalloc(bytes);
	} // new

	void Layout::operator delete(void *memory)
	{
		_gfree(memory);
	} // delete

	struct LayoutInfo
	{
		Point origin;
		Size size;
		Panel* panel;
	};

	//TestLayout::TestLayout()
	//{
	//	reset_colors();
	//}

	//TestLayout::~TestLayout()
	//{
	//}

	//void TestLayout::reset_colors()
	//{
	//	for (size_t index = 0; index < 5; ++index)
	//	{
	//		colors[index] = gemini::Color(0.0f, 0.0f, 0.0f, 0.0f);
	//	}
	//}

	//void TestLayout::add_panel(Panel* panel)
	//{
	//	LayoutRecord item;
	//	item.type = LayoutItem_Panel;
	//	item.object = panel;
	//	items.push_back(item);
	//}

	//void TestLayout::add_layout(Layout* layout)
	//{
	//	LayoutRecord item;
	//	item.type = LayoutItem_Layout;
	//	item.object = layout;
	//	items.push_back(item);
	//}

	//void TestLayout::update(Panel* parent, PanelVector& children)
	//{
	//	Size size = parent->get_size();

	//	float width = size.width;
	//	float height = size.height;

	//	float side_width = (width * 0.15f);
	//	float rect_width = 2 * side_width;
	//	float center_width = width - rect_width;

	//	float vert_height = (height * 0.15f);
	//	float rect_height = 2 * vert_height;
	//	float center_height = height - rect_height;

	//	Point leftover = Point(center_width + side_width, center_height + vert_height);

	//	Point topleft = Point(side_width, 0.0f);

	//	Point pix = Point(side_width, vert_height);

	//	Point cw = Point(center_width, center_height);

	//	// left
	//	regions[0].set(0.0f, 0.0f, pix.x, size.height);

	//	// top
	//	regions[1].set(topleft.x, 0.0f, cw.x, pix.y);

	//	// right
	//	regions[2].set(leftover.x, 0.0f, pix.x, size.height);

	//	// bottom
	//	regions[3].set(topleft.x, leftover.y, cw.x, pix.y);

	//	// center
	//	regions[4].set(pix.x, pix.y, cw.x, cw.y);

	//	recursive_update(this);

	//	Size dimensions = parent->get_size();
	//	Point origin(0, 32);

	//	dimensions.height -= origin.y;
	//	float item_height = (dimensions.height / items.size());

	//	for (size_t index = 0; index < items.size(); ++index)
	//	{
	//		LayoutRecord& record = items[index];
	//		if (record.type == LayoutItem_Panel)
	//		{
	//			Panel* panel = (Panel*)record.object;
	//			panel->set_origin(0, origin.y);
	//			float height = item_height;// glm::min(item_height, record.maximum.height);
	//			panel->set_size(dimensions.width, height);
	//			origin.y += height;
	//			dimensions.height -= height;
	//		}
	//	}
	//} // update

	//void TestLayout::recursive_update(TestLayout* layout)
	//{
	//	for (size_t index = 0; index < items.size(); ++index)
	//	{
	//		LayoutRecord& record = items[index];
	//		record.minimum = Size(0, 0);
	//		record.maximum = Size(0, 0);
	//		record.desired = Size(0, 0);

	//		if (record.type == LayoutItem_Layout)
	//		{
	//			recursive_update((TestLayout*)record.object);
	//		}

	//		if (record.type == LayoutItem_Panel)
	//		{
	//			Panel* panel = (Panel*)record.object;
	//			record.desired = panel->get_size();
	//			record.maximum.height = panel->get_size().height;
	//		}
	//	}
	//} // recursive_update

	//void TestLayout::render(Compositor* compositor, Panel* panel, gui::render::CommandList& render_commands)
	//{
	//	// draw all regions
	//	for (size_t index = 0; index < 5; ++index)
	//	{
	//		gui::Point region[4];
	//		region[0] = transform_point(panel->get_transform(0), Point(regions[index].origin.x, regions[index].origin.y + regions[index].size.height));
	//		region[1] = transform_point(panel->get_transform(0), Point(regions[index].origin.x + regions[index].size.width, regions[index].origin.y + regions[index].size.height));
	//		region[2] = transform_point(panel->get_transform(0), Point(regions[index].origin.x + regions[index].size.width, regions[index].origin.y));
	//		region[3] = transform_point(panel->get_transform(0), Point(regions[index].origin.x, regions[index].origin.y));

	//		render_commands.add_rectangle(
	//			region[0],
	//			region[1],
	//			region[2],
	//			region[3],
	//			render::WhiteTexture,
	//			colors[index]
	//		);
	//	}
	//} // render

	//bool TestLayout::hit_test_local(const Point& local_point) const
	//{
	//	return false;
	//} // hit_test_local

	//void TestLayout::handle_event(EventArgs& args)
	//{
	//	if (args.type == Event_CursorMove)
	//	{
	//		reset_colors();

	//		for (size_t index = 0; index < 5; ++index)
	//		{
	//			if (regions[index].is_point_inside(args.local))
	//			{
	//				//if (args.compositor->get_capture())
	//				{
	//					//LOGV("found an event\n");
	//					colors[index] = gemini::Color(0.0f, 0.25f, 0.5f, 0.5f);
	//					break;
	//				}
	//			}
	//		}

	//		//args.handled = 1;
	//	}
	//	else if (args.type == Event_CursorDragEnter)
	//	{
	//	}
	//	else if (args.type == Event_CursorDragExit)
	//	{
	//		reset_colors();
	//	}
	//	else if (args.type == Event_CursorExit)
	//	{
	//		reset_colors();
	//	}
	//} // handle_event


	//
	// Spacer
	//

	const Size& Spacer::get_size() const
	{
		return size;
	} // get_size

	void Spacer::set_size(const Size& new_size)
	{
		size = new_size;
	} // set_size

	void* Spacer::operator new(size_t bytes)
	{
		return _gmalloc(bytes);
	} // new

	void Spacer::operator delete(void *memory)
	{
		_gfree(memory);
	} // delete

	//
	// Box Layout
	//
	BoxLayout::~BoxLayout()
	{
		clear_children();
	}


	void BoxLayout::add_panel(Panel* panel)
	{
		LayoutRecord item;
		item.type = LayoutItem_Panel;
		item.object = panel;
		items.push_back(item);
	} // add_panel

	void BoxLayout::add_layout(Layout* layout)
	{
		LayoutRecord item;
		item.type = LayoutItem_Layout;
		item.object = layout;
		items.push_back(item);
	} // add_layout

	void BoxLayout::add_spacer(Spacer* spacer)
	{
		LayoutRecord item;
		item.type = LayoutItem_Spacer;
		item.object = spacer;
		items.push_back(item);
	} // add_spacer

	void BoxLayout::recursive_update(const Size& size, size_t& fixed_size_children, size_t& visible_children)
	{
		fixed_size = Size(0, 0);

		for (size_t index = 0; index < items.size(); ++index)
		{
			LayoutRecord& record = items[index];
			//record.minimum = Size(0, 0);
			//record.maximum = Size(0, 0);
			//record.desired = Size(0, 0);
			//record.fixed = Size(0, 0);

			if (record.type == LayoutItem_Panel)
			{
				Panel* panel = static_cast<Panel*>(record.object);
				if (!panel->has_flags(Panel::Flag_IsVisible))
				{
					continue;
				}
			}

			if (record.type == LayoutItem_Spacer)
			{
				Spacer* spacer = static_cast<Spacer*>(record.object);
				fixed_size += spacer->get_size();
				//record.fixed = spacer->get_size();
				++fixed_size_children;
			}

			++visible_children;
		}
	} // recursive_update

	void BoxLayout::update(Panel* parent, PanelVector& children)
	{
		Size dimensions = parent->get_size();
		Point child_origin(0, 0);
		dimensions.width -= child_origin.x;
		dimensions.height -= child_origin.y;

		update_children(child_origin, dimensions);
	} // update

	void BoxLayout::clear_children()
	{
		// Iterate over items in the layout.
		// The layout is responsible for removing child spacers and layouts.
		for (size_t index = 0; index < items.size(); ++index)
		{
			const LayoutRecord& record = items[index];
			if (record.type == LayoutItem_Spacer)
			{
				Spacer* spacer = static_cast<Spacer*>(record.object);
				delete spacer;
			}
			else if (record.type == LayoutItem_Layout)
			{
				BoxLayout* layout = static_cast<BoxLayout*>(record.object);
				delete layout;
			}
		}
		items.clear();
	} // clear_children

	//
	// Horizontal Layout
	//
	void HorizontalLayout::update_children(Point& child_origin, Size size)
	{
		size_t fixed_size_children = 0;
		size_t visible_children = 0;
		recursive_update(size, fixed_size_children, visible_children);
		float item_width = (size.width - fixed_size.width) / (visible_children - fixed_size_children);
		for (size_t index = 0; index < items.size(); ++index)
		{
			float prev_origin_y = child_origin.y;
			LayoutRecord& record = items[index];
			if (record.type == LayoutItem_Panel)
			{
				Panel* panel = static_cast<Panel*>(record.object);
				if (!panel->has_flags(Panel::Flag_IsVisible))
				{
					continue;
				}

				panel->set_origin(child_origin.x, child_origin.y);
				panel->set_size(item_width, size.height);
				size.width -= item_width;
				child_origin.x += item_width;
			}
			else if (record.type == LayoutItem_Spacer)
			{
				Spacer* spacer = static_cast<Spacer*>(record.object);
				child_origin.x += spacer->get_size().width;
			}
			else if (record.type == LayoutItem_Layout)
			{
				BoxLayout* layout = static_cast<BoxLayout*>(record.object);
				layout->update_children(child_origin, Size(item_width, size.height));
				child_origin.x += item_width;
			}
			child_origin.y = prev_origin_y;
		}
	} // update_children

	//
	// Vertical Layout
	//
	void VerticalLayout::update_children(Point& child_origin, Size size)
	{
		size_t fixed_size_children = 0;
		size_t visible_children = 0;
		recursive_update(size, fixed_size_children, visible_children);
		float item_height = (size.height - fixed_size.height) / (visible_children - fixed_size_children);
		for (size_t index = 0; index < items.size(); ++index)
		{
			float prev_origin_x = child_origin.x;
			LayoutRecord& record = items[index];
			if (record.type == LayoutItem_Panel)
			{
				Panel* panel = static_cast<Panel*>(record.object);
				if (!panel->has_flags(Panel::Flag_IsVisible))
				{
					continue;
				}

				panel->set_origin(child_origin.x, child_origin.y);
				panel->set_size(size.width, item_height);
				size.height -= item_height;
				child_origin.y += item_height;
			}
			else if (record.type == LayoutItem_Spacer)
			{
				Spacer* spacer = static_cast<Spacer*>(record.object);
				child_origin.y += spacer->get_size().height;
			}
			else if (record.type == LayoutItem_Layout)
			{
				BoxLayout* layout = static_cast<BoxLayout*>(record.object);
				layout->update_children(child_origin, Size(size.width, item_height));
				child_origin.y += item_height;
			}
			child_origin.x = prev_origin_x;
		}
	} // update_children
} // namespace gui

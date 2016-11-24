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
#pragma once

//#include <stdio.h>
//#include <string>
//#include <stdint.h>

#include <ui/utils.h>

#include <core/typedefs.h>
#include <core/typespec.h>

namespace gui
{
	class Compositor;
	class Panel;

	class Layout
	{
		TYPESPEC_DECLARE_CLASS_NOBASE(Layout);

	public:
		Layout();
		virtual ~Layout();
		virtual void update(Panel* parent, PanelVector& children) = 0;
		virtual void render(Compositor* compositor, Panel* panel, gui::render::CommandList& render_commands) = 0;
		virtual bool hit_test_local(const Point& local_point) const = 0;
		virtual void handle_event(EventArgs& args) = 0;

		// memory overrides
		void* operator new(size_t bytes);
		void operator delete(void* memory);

		virtual void clear_children() = 0;
	}; // class Layout

	enum LayoutItem
	{
		LayoutItem_Panel,
		LayoutItem_Layout,
		LayoutItem_Spacer
	};

	struct LayoutRecord
	{
		LayoutItem type;
		void* object;

		Size item_size;
	};

	//class TestLayout : public Layout
	//{
	//private:
	//	// left, right, top, bottom, center
	//	gui::Rect regions[5];
	//	gemini::Color colors[5];

	//	void reset_colors();

	//	Array<LayoutRecord> items;

	//public:
	//	TestLayout();
	//	~TestLayout();


	//	void add_panel(Panel* panel);
	//	void add_layout(Layout* layout);

	//	void recursive_update(TestLayout* layout);

	//	virtual void update(Panel* parent, PanelVector& children) override;
	//	virtual void render(Compositor* compositor, Panel* panel, gui::render::CommandList& render_commands) override;
	//	virtual bool hit_test_local(const Point& local_point) const override;
	//	virtual void handle_event(EventArgs& args) override;

	//}; // class TestLayout

	class Spacer
	{
	public:
		TYPESPEC_DECLARE_CLASS_NOBASE(Spacer);

		enum Direction
		{
			Vertical,
			Horizontal
		};


		const Size& get_size() const;
		void set_size(const Size& size);

		// memory overrides
		void* operator new(size_t bytes);
		void operator delete(void* memory);

	private:
		// Only fixed sizes are supported.
		Size size;
	};

	class BoxLayout : public Layout
	{
		TYPESPEC_DECLARE_CLASS(BoxLayout, Layout);

	protected:

		enum LayoutDirection
		{
			Layout_Vertical,
			Layout_Horizontal
		};

		LayoutDirection direction;
		Array<LayoutRecord> items;
		Size fixed_size;

		void recursive_update(const Size& size, size_t& fixed_size_children, size_t& visible_children);

	public:
		virtual ~BoxLayout();

		void add_panel(Panel* panel);
		void add_layout(Layout* layout);
		void add_spacer(Spacer* spacer);

		virtual void update(Panel* parent, PanelVector& children) override;
		virtual void render(Compositor* compositor, Panel* panel, gui::render::CommandList& render_commands) override {}
		virtual bool hit_test_local(const Point& local_point) const override { return false; }
		virtual void handle_event(EventArgs& args) override {}

		virtual void update_children(Point& origin, Size size) = 0;

		virtual void clear_children() override;
	};

	class HorizontalLayout : public BoxLayout
	{
		TYPESPEC_DECLARE_CLASS(HorizontalLayout, BoxLayout);
	public:
		virtual void update_children(Point& origin, Size size) override;
	};

	class VerticalLayout : public BoxLayout
	{
		TYPESPEC_DECLARE_CLASS(VerticalLayout, BoxLayout);
	public:
		virtual void update_children(Point& origin, Size size) override;
	};




	class DockingLayout : public Layout
	{
		TYPESPEC_DECLARE_CLASS(DockingLayout, Layout);

	public:
	};
} // namespace gui

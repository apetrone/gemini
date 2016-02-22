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
#include "ui/ui.h"
#include "ui/panel.h"
#include "ui/compositor.h"

#include <core/logging.h>

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

	HBoxLayout::HBoxLayout()
		: left_margin(4.0f)
		, right_margin(4.0f)
		, top_margin(4.0f)
	{
	}

	HBoxLayout::~HBoxLayout()
	{
	}

	void arrange_panels(Array<LayoutInfo>& layout_data,
		const Size& parent_size,
		size_t row_width,
		size_t available_height,
		size_t first_row_panel,
		size_t last_row_panel,
		Point& origin)
	{
		// divide this by the number of panels on this row
		const float per_child_space = (row_width / static_cast<float>(last_row_panel - first_row_panel));
		float max_height = 0.0f;

		for (size_t current_index = first_row_panel; current_index < last_row_panel; ++current_index)
		{
			LayoutInfo& edit = layout_data[current_index];
			edit.size.width += per_child_space;
			edit.panel->set_origin(origin.x, origin.y);
			edit.panel->set_dimensions(
				edit.size.width / parent_size.width,
				edit.size.height / parent_size.height
			);
			origin.x += edit.size.width;
			max_height = glm::max(edit.size.height, max_height);
		}

		origin.y += max_height;
		available_height -= static_cast<size_t>(max_height);
	}

	void HBoxLayout::update(Panel* parent, PanelVector& children)
	{
		const Size parent_size = parent->get_size();
		const size_t total_children = children.size();
		const float per_child = 1.0f / static_cast<float>(total_children);
		Size remaining_fill = parent->get_size();

		DimensionType row_width = 0;
		size_t available_height = 0;

		remaining_fill.width -= (left_margin + right_margin);
		remaining_fill.height -= top_margin;

		available_height = static_cast<size_t>(remaining_fill.height);

		if (!children.empty())
		{
			Point origin(left_margin, top_margin);

			Array<LayoutInfo> layout_data;
			layout_data.resize(total_children);

			size_t index = 0;
			for (Panel* child : children)
			{
				LayoutInfo& info = layout_data[index];
				child->measure(info.size);
				info.panel = child;
				row_width += info.size.width;
				++index;
			}

			// calculate target_sizes

			index = 0;
			size_t first_row_panel = 0;
			size_t last_row_panel = 0;
			row_width = remaining_fill.width;

			Point row_origin(left_margin, top_margin);

			for (size_t panel_index = 0; panel_index < total_children; ++panel_index)
			{
				LayoutInfo& info = layout_data[index];

				if ((origin.y + info.size.height) > available_height)
				{
					// This layout cannot fit any more panels vertically.
					// TODO: Perhaps add a scrollable panel, instead?
					assert(0);
				}

				// If we can fit this panel in the row...
				if (row_width >= info.size.width)
				{
					row_width -= info.size.width;
					info.panel->set_origin(origin.x, origin.y);
					info.panel->set_dimensions(
						info.size.width / parent_size.width,
						info.size.height / parent_size.height
					);
				}
				else
				{
					// We cannot fit this panel. Re-arrange the previous items
					// in the row.
					arrange_panels(layout_data,
						parent_size,
						static_cast<size_t>(row_width),
						available_height,
						first_row_panel,
						last_row_panel,
						origin
					);

					// reset the origin.x
					origin.x = left_margin;

					row_width = remaining_fill.width;

					// try to fit this panel again.
					first_row_panel = panel_index;
					panel_index--;
					last_row_panel = panel_index;
					continue;
				}

				++index;
				last_row_panel = index;
			}

			// re-arrange the last row since it didn't fit as snugly.
			if (row_width > 0)
			{
				arrange_panels(layout_data,
					parent_size,
					static_cast<size_t>(row_width),
					available_height,
					first_row_panel,
					last_row_panel,
					origin
				);
			}
		}
	}
}

namespace gui
{
	void* Panel::operator new(size_t bytes)
	{
		return _gmalloc(bytes);
	} // new

	void Panel::operator delete(void *memory)
	{
		_gfree(memory);
	} // delete

	Panel::Panel(Panel* parent_panel)
	{
		z_rotation = 0.0;
		scale[0] = 1;
		scale[1] = 1;
		z_depth = 0;
		parent = parent_panel;
		userdata = 0;
		background_color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		foreground_color = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
		flags = (Flag_CursorEnabled | Flag_TransformIsDirty | Flag_NeedsLayout);
		set_visible(true);
		layout = nullptr;
		set_dimensions(1.0f, 1.0f);

		// default: no maximum size
		maximum_size = Size(0, 0);

		if (parent)
		{
			parent->add_child(this);
		}
	} // Panel

	Panel::~Panel()
	{
		PanelVector::iterator it, end;
		it = children.begin();
		end = children.end();
		for(; it != end; ++it)
		{
			Panel* panel = (*it);
			panel->~Panel();
			gui::_gfree(panel);
		}

		children.clear();

		if (layout)
		{
			layout->~Layout();
			gui::_gfree(layout);
			layout = nullptr;
		}
	} // ~Panel

	void Panel::set_bounds(const ScreenInt x, const ScreenInt y, const DimensionType width, const DimensionType height)
	{
		origin.x = x;
		origin.y = y;
		size.width = width;
		size.height = height;
		flags |= Flag_TransformIsDirty;
	} // set_bounds

	void Panel::set_bounds(const Rect& new_bounds)
	{
		origin = new_bounds.origin;
		size = new_bounds.size;
	} // set_bounds

	void Panel::set_dimensions(float x, float y)
	{
		assert(x <= 1.0f);
		assert(y <= 1.0f);
		dimensions.x = x;
		dimensions.y = y;

		update_size_from_dimensions();

		flags |= Flag_TransformIsDirty;
	} // set_dimensions

	void Panel::set_dimensions(const Point& new_dimensions)
	{
		dimensions = new_dimensions;

		assert(parent);

		update_size_from_dimensions();

		flags |= Flag_TransformIsDirty;
	} // set_dimensions

	void Panel::set_origin(float x, float y)
	{
		origin.x = x;
		origin.y = y;
		flags |= Flag_TransformIsDirty;
	} // set_origin

	void Panel::set_size(const Size& new_size)
	{
		size = new_size;

		// re-compute dimensions
		dimensions.x = (new_size.width / parent->size.width);
		dimensions.y = (new_size.height / parent->size.height);

		flags |= Flag_TransformIsDirty;
	}

	void Panel::get_content_bounds(Rect& content_bounds) const
	{
		content_bounds.origin = origin;
		content_bounds.size = size;
	} // get_content_bounds

	void Panel::add_child(Panel* panel)
	{
		children.push_back(panel);
	} // add_child

	void Panel::remove_child(Panel* panel)
	{
		PanelVector::iterator it, end;
		it = children.begin();
		end = children.end();
		for(; it != end; ++it)
		{
			if ((*it) == panel)
			{
				children.erase(it);
				break;
			}
		}
	} // remove_child

	void Panel::handle_event(EventArgs& args)
	{
		if (has_flags(Flag_CursorEnabled))
		{
			if (args.type == Event_CursorDrag)
			{
				if (has_flags(Flag_CanMove))
				{
					origin.x += args.delta.x;
					origin.y += args.delta.y;
					flags |= Flag_TransformIsDirty;
					args.handled = true;
				}
			}
		}

		// send event up the chain
		if (!args.handled && parent)
		{
			parent->handle_event(args);
		}
	} // handle_event

	void Panel::update(Compositor* compositor, float delta_seconds)
	{
		update_transform(compositor);

		if (layout && (flags & Flag_NeedsLayout))
		{
			layout->update(this, children);
			flags &= ~Flag_NeedsLayout;
		}

		for(PanelVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			(*it)->update(compositor, delta_seconds);
		}

		if (flags & Flag_TransformIsDirty)
		{
			flags &= ~Flag_TransformIsDirty;
		}
	} // update

	void Panel::render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands)
	{
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			background_color);

		if (this->background.is_valid())
		{
//			renderer->draw_textured_bounds(frame, this->background);
			render_commands.add_rectangle(
				geometry[0],
				geometry[1],
				geometry[2],
				geometry[3],
				this->background,
				gemini::Color::from_rgba(255, 255, 255, 255)
			);
		}

		render_children(compositor, renderer, render_commands);
	} // render

	void Panel::render_children(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands)
	{
		for(PanelVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			Panel* child = (*it);
			if (child->is_visible())
			{
				child->render(compositor, renderer, render_commands);
			}
		}
	} // render_children

	void Panel::set_background_image(Compositor* /*compositor*/, const char* /*path*/)
	{
		// This needs to be updated to use the new ResourceCache
		assert(0);
//		if (compositor && compositor->get_renderer())
//		{
//			compositor->renderer->texture_create(path, background);
//		}
	} // set_background_image


	void Panel::set_background_color(const gemini::Color& color)
	{
		background_color = color;
	}

	void Panel::set_foreground_color(const gemini::Color& color)
	{
		foreground_color = color;
	}

	void Panel::set_visible(bool is_visible)
	{
		this->visible = is_visible;
		if (is_visible)
		{
			flags |= Flag_IsVisible;
		}
		else
		{
			flags &= ~Flag_IsVisible;
		}
	} // set_visible

	bool Panel::is_visible() const
	{
		return this->visible;
	} // is_visible

	void Panel::measure(Size& minimum_size) const
	{
		static size_t index = 0;
		minimum_size.width = 100;
		minimum_size.height = 30;

		index++;
	}

	void Panel::resize(const Size& requested_size)
	{
		if (parent)
		{
			const Size& parent_size = parent->get_size();
			set_dimensions(
				(requested_size.width / parent_size.width),
				(requested_size.height / parent_size.height)
			);
		}
		else
		{
			set_dimensions(1.0f, 1.0f);
		}

		size = requested_size;
	}

	void Panel::set_maximum_size(const Size& max_size)
	{
		assert(parent != nullptr);


		maximum_size = max_size;
	}

	bool Panel::hit_test_local(const Point& local_point) const
	{
		// Don't use 'bounds' here because those are
		// parent-space bounds.
		// Setup local bounds here with origin (0, 0).
		Rect local_bounds(Point(0.0f), size);
		return local_bounds.is_point_inside(local_point);
	}

	// ---------------------------------------------------------------------
	// transforms
	// ---------------------------------------------------------------------
	void Panel::set_rotation(const float radians)
	{
		z_rotation = radians;
		flags |= Flag_TransformIsDirty;
	}

	void Panel::set_scale(const glm::vec2& new_scale)
	{
		scale = new_scale;
		flags |= Flag_TransformIsDirty;
	}

	// ---------------------------------------------------------------------
	// other utils
	// ---------------------------------------------------------------------
	Compositor* Panel::get_compositor()
	{
		Panel* panel = parent;
		while(parent && panel->parent)
			panel = panel->parent;

		assert(panel);
		return static_cast<Compositor*>(panel);
	}

	Point Panel::pixels_from_dimensions(const Point& input_dimensions) const
	{
		assert(parent);
		Point pixels;
		pixels.x = (input_dimensions.x * parent->size.width);
		pixels.y = (input_dimensions.y * parent->size.height);
		return pixels;
	}

	Point Panel::dimensions_from_pixels(const Point& pixels) const
	{
		assert(parent);
		Point out_dimensions;
		out_dimensions.x = (pixels.x / parent->size.width);
		out_dimensions.y = (pixels.y / parent->size.height);
		return out_dimensions;
	}

	Point Panel::compositor_to_local(const Point& location)
	{
		const glm::mat3 transform = get_transform(0);
		return glm::vec2(glm::inverse(transform) * glm::vec3(location, 1.0));
	}

	glm::mat3 Panel::get_transform(size_t /*index*/) const
	{
		return local_transform;
	}

	void Panel::update_size_from_dimensions()
	{
		// Uh-oh, did you forget to set_dimensions on this panel?
		assert(dimensions.x > 0 && dimensions.y > 0);

		// Most panels have a parent, but the compositor does NOT.
		Size parent_size = size;
		if (parent)
		{
			parent_size = parent->size;
		}
		size.width = (dimensions.x * parent_size.width);
		size.height = (dimensions.y * parent_size.height);
	}

	void Panel::set_layout(Layout* layout_instance)
	{
		// If you hit this, this panel already has a layout.
		// What should we do here? Replace the old one?
		assert(layout == nullptr);

		layout = layout_instance;
	}

	void Panel::update_transform(Compositor*)
	{
		// Really? How do you not have a parent?
		assert(parent);

		if (!is_visible())
			return;

		if ((parent->get_flags() & Flag_TransformIsDirty) || (flags & Flag_TransformIsDirty))
		{
			flags |= Flag_TransformIsDirty;

			// update the local_transform matrix
			glm::mat3 parent_transform;
			if (parent)
			{
				parent_transform = parent->get_transform(0);

				// recalculate the size based on dimensions
				assert(dimensions.x > mathlib::EPSILON);
				size.width = (dimensions.x * parent->size.width);
				assert(size.width != 0);

				assert(dimensions.y > mathlib::EPSILON);
				size.height = (dimensions.y * parent->size.height);
				assert(size.height != 0);
			}

			update_size_from_dimensions();

			// make sure we clamp the dimensions here
			if (maximum_size.width != 0)
			{
				size.width = glm::min(size.width, maximum_size.width);
			}

			if (maximum_size.height != 0)
			{
				size.height = glm::min(size.height, maximum_size.height);
			}

			// the points have to be rotated around the center pivot
			Point center(size.width/2, size.height/2);

			const glm::mat3 inverse_pivot = translate_matrix(-center);
			const glm::mat3 pivot = translate_matrix(center);
			const glm::mat3 translation = translate_matrix(origin);

			// from RIGHT to LEFT:
			// transform to origin (using inverse pivot)
			// perform locale scale, then rotation
			// transform back by pivot
			local_transform = translation * pivot * glm::mat3(rotation_matrix(z_rotation) * scale_matrix(scale)) * inverse_pivot * parent_transform;

			// geometry in local coordinates
			geometry[0] = Point(0, 0);
			geometry[1] = Point(0, size.height);
			geometry[2] = Point(size.width, size.height);
			geometry[3] = Point(size.width, 0);

			geometry[0] = transform_point(local_transform, geometry[0]);
			geometry[1] = transform_point(local_transform, geometry[1]);
			geometry[2] = transform_point(local_transform, geometry[2]);
			geometry[3] = transform_point(local_transform, geometry[3]);
		}
	}

} // namespace gui

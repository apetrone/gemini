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
#include <ui/layout.h>

#include <core/logging.h>

#include <algorithm> // for std::sort

TYPESPEC_REGISTER_CLASS(gui::Panel);

namespace gui
{
	struct ZSort_Panel_Descending
	{
		bool operator()(Panel* left, Panel* right)
		{
			// This must obey strict weak ordering; so we'll have to come up
			// with another plan.
			return (left->z_depth < right->z_depth);
		}
	}; // ZSort_Panel_Descending

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
		z_rotation = 0.0f;
		scale[0] = 1;
		scale[1] = 1;
		z_depth = 0;
		parent = nullptr;
		userdata = 0;
		background_color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		foreground_color = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
		flags = (Flag_CursorEnabled | Flag_TransformIsDirty | Flag_NeedsLayout);
		set_visible(true);
		layout = nullptr;
		set_size(0, 0);

		// default: no maximum size
		maximum_size = Size(0, 0);

		if (parent_panel)
		{
			parent_panel->add_child(this);
		}
	} // Panel

	Panel::~Panel()
	{
		clear_children();

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

	void Panel::set_origin(float x, float y)
	{
		origin.x = x;
		origin.y = y;
		flags |= Flag_TransformIsDirty;
	} // set_origin

	void Panel::set_origin(const Point& new_origin)
	{
		set_origin(new_origin.x, new_origin.y);
	} // set_origin

	void Panel::set_size(const Size& new_size)
	{
		size = new_size;
		flags |= Flag_TransformIsDirty;
	} // set_size

	void Panel::set_size(uint32_t width, uint32_t height)
	{
		set_size(Size(width, height));
	} // set_size

	void Panel::swap_child(size_t first_index, size_t second_index)
	{
		Panel* temp = children[second_index];
		children[second_index] = children[first_index];
		children[first_index] = temp;
	} // swap_child

	void Panel::get_content_bounds(Rect& content_bounds) const
	{
		content_bounds.origin = origin;
		content_bounds.size = size;
	} // get_content_bounds

	void Panel::add_child(Panel* panel)
	{
		if (panel->parent != this)
		{
			if (panel->parent)
			{
				panel->parent->remove_child(panel);
			}

			panel->parent = this;
			children.push_back(panel);
		}

		panel->flags |= Flag_TransformIsDirty;

		// add panel to zsorted list
		zsorted.push_back(panel);
		zsort_children(panel);

		assert(panel->parent);
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

		it = zsorted.begin();
		end = zsorted.end();
		for (; it != end; ++it)
		{
			if ((*it) == panel)
			{
				zsorted.erase(it);
				break;
			}
		}
	} // remove_child

	void Panel::handle_event(EventArgs& args)
	{
		if (args.type == Event_CursorDrag && get_parent())
		{
			origin.x += args.delta.x;
			origin.y += args.delta.y;
			flags |= Flag_TransformIsDirty;
			args.handled = true;
			LOGV("Moved panel %s to %2.2f, %2.2f\n", get_name(), origin.x, origin.y);
		}
//		else if (!args.handled && args.type == Event_CursorButtonPressed)
//		{
//			LOGV("set focus to panel %s\n", get_name());
//			get_compositor()->set_focus(this);
//		}
	} // handle_event

	void Panel::update(Compositor* compositor, float delta_seconds)
	{
		update_transform(compositor);

		if (layout && ((flags & Flag_NeedsLayout) || flags & Flag_TransformIsDirty))
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

	void Panel::render_geometry(gui::render::CommandList& render_commands, const gemini::Color& color)
	{
		render_commands.add_rectangle(
			geometry[0],
			geometry[1],
			geometry[2],
			geometry[3],
			render::WhiteTexture,
			color);
	} // render_geometry

	void Panel::render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands)
	{
		render_geometry(render_commands, background_color);

		const uint32_t capture_flags = (Flag_CursorEnabled | Flag_CanMove);
		if ((flags & (capture_flags)) == capture_flags)
		{

			Point rects[4];
			rects[0] = Point(0, 0);
			rects[1] = Point(0, capture_rect.size.height);
			rects[2] = Point(capture_rect.size.width, capture_rect.size.height);
			rects[3] = Point(capture_rect.size.width, 0);

			rects[0] = transform_point(local_transform, rects[0]);
			rects[1] = transform_point(local_transform, rects[1]);
			rects[2] = transform_point(local_transform, rects[2]);
			rects[3] = transform_point(local_transform, rects[3]);

			render_commands.add_rectangle(
				rects[0],
				rects[1],
				rects[2],
				rects[3],
				render::WhiteTexture,
				gemini::Color::from_rgba(32, 32, 32, 64));
		}

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
		for (PanelVector::reverse_iterator it = zsorted.rbegin(); it != zsorted.rend(); ++it)
		{
			Panel* panel = (*it);
			if (panel->is_visible())
			{
				panel->render(compositor, renderer, render_commands);

				Layout* layout = panel->get_layout();
				if (layout)
				{
					layout->render(compositor, panel, render_commands);
				}
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
			flags |= Flag_TransformIsDirty;
		}
		else
		{
			flags &= ~Flag_IsVisible;
			flags &= ~Flag_TransformIsDirty;
		}
	} // set_visible

	bool Panel::is_visible() const
	{
		return this->visible;
	} // is_visible

	size_t Panel::total_children() const
	{
		return children.size();
	} // total_children

	Panel* Panel::child_at(size_t index)
	{
		assert(index < total_children());
		return children[index];
	} // child_at

	Panel* Panel::child_at(size_t index) const
	{
		assert(index < total_children());
		return children[index];
	} // child_at

	void Panel::clear_children()
	{
		PanelVector::iterator it, end;
		it = children.begin();
		end = children.end();
		for (; it != end; ++it)
		{
			Panel* panel = (*it);
			panel->~Panel();
			gui::_gfree(panel);
		}

		children.clear();
		zsorted.clear();

		if (get_layout())
		{
			get_layout()->clear_children();
		}
	} // clear_children

	void Panel::measure(Size& minimum_size) const
	{
		static size_t index = 0;
		minimum_size.width = 100;
		minimum_size.height = 30;

		index++;
	}

	void Panel::resize(const Size& requested_size)
	{
		size = requested_size;
		flags |= Flag_TransformIsDirty;
	}

	void Panel::set_maximum_size(const Size& max_size)
	{
		assert(parent != nullptr);
		maximum_size = max_size;
	}

	void Panel::bring_to_front()
	{
		// If you hit this, this panel has no parent and cannot be re-ordered.
		assert(get_parent());

		// Compositor children, (at least for now) must be ordered differently.
		assert(get_parent() != get_compositor());

		if (can_send_to_front())
		{
			get_parent()->zsort_children(this);
		}
	} // bring_to_front

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

	bool Panel::point_in_capture_rect(const Point& local) const
	{
		return capture_rect.is_point_inside(local);
	} // is local point in the panel's capture rect?

	Point Panel::compositor_to_local(const Point& location)
	{
		const glm::mat3 transform = get_transform(0);
		return glm::vec2(glm::inverse(transform) * glm::vec3(location, 1.0));
	}

	glm::mat3 Panel::get_transform(size_t /*index*/) const
	{
		return local_transform;
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
			// update the local_transform matrix
			glm::mat3 parent_transform;
			if (parent)
			{
				parent_transform = parent->get_transform(0);

				// recalculate the size based on dimensions
				// assert(dimensions.x > mathlib::EPSILON);
				// size.width = (dimensions.x * parent->size.width);
				assert(size.width != 0);

				// assert(dimensions.y > mathlib::EPSILON);
				// size.height = (dimensions.y * parent->size.height);
				assert(size.height != 0);
			}

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

			capture_rect.set(0, 0, size.width, 16);
		}
	} // update_transform

	void Panel::zsort_children(Panel* panel)
	{
		if (!panel)
		{
			return;
		}

		// reset thi panel's zdepth
		panel->z_depth = 0;

		size_t zdepth = 1;

		// search the zlist for the top window
		for (PanelVector::iterator it = zsorted.begin(); it != zsorted.end(); ++it)
		{
			Panel* target = (*it);
			if (target != panel)
			{
				target->z_depth = zdepth++;
			}
		}

		std::sort(zsorted.begin(), zsorted.end(), ZSort_Panel_Descending());
	} // zsort_children

} // namespace gui

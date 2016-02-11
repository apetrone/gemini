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
	void* Panel::operator new(size_t bytes)
	{
		return _gmalloc(bytes);
	} // new

	void Panel::operator delete(void *memory)
	{
		_gfree(memory);
	} // delete

	Panel::Panel(Panel* parent)
	{
		this->z_rotation = 0.0;
		scale[0] = 1;
		scale[1] = 1;
		this->z_depth = 0;
		this->parent = parent;
		this->userdata = 0;
		this->background_color = gemini::Color(1.0f, 1.0f, 1.0f, 1.0f);
		this->foreground_color = gemini::Color(0.0f, 0.0f, 0.0f, 1.0f);
		this->flags = (Flag_CursorEnabled | Flag_TransformIsDirty);
		set_visible(true);

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
		dimensions.x = x;
		dimensions.y = y;

		assert(parent);

		assert(x <= 1.0f);
		assert(y <= 1.0f);
		size.width = (dimensions.x * parent->size.width);
		size.height = (dimensions.y * parent->size.height);

		flags |= Flag_TransformIsDirty;
	} // set_dimensions

	void Panel::set_dimensions(const Point& new_dimensions)
	{
		dimensions = new_dimensions;

		assert(parent);

		size.width = (dimensions.x * parent->size.width);
		size.height = (dimensions.y * parent->size.height);

		flags |= Flag_TransformIsDirty;
	} // set_dimensions

	void Panel::set_origin(float x, float y)
	{
		origin.x = x;
		origin.y = y;
		flags |= Flag_TransformIsDirty;
	} // set_origin

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
				}
			}
			else if (args.type == Event_CursorButtonReleased)
			{
				if (args.cursor_button == gui::CursorButton::Middle)
				{
					LOGV("bounds = {%2.2f, %2.2f, %g, %g}\n",
						origin.x,
						origin.y,
						size.width,
						size.height
					);
				}
			}
		}
	} // handle_event

	void Panel::update(Compositor* compositor, float delta_seconds)
	{
		update_transform(compositor);

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

	void Panel::update_transform(Compositor* compositor)
	{
		if (parent && parent->get_flags() & Flag_TransformIsDirty)
		{
			flags |= Flag_TransformIsDirty;
		}

		// update the local_transform matrix
		if (flags & Flag_TransformIsDirty)
		{
			glm::mat3 parent_transform;
			if (parent && parent != compositor)
			{
				parent_transform = parent->get_transform(0);
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

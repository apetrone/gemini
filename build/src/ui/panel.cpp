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

namespace gui
{
	void* Panel::operator new(size_t bytes)
	{
		return _gmalloc(bytes);
	}
	
	void Panel::operator delete(void *memory)
	{
		_gfree(memory);
	}

	Panel::Panel( Panel * parent )
	{
		fprintf(stdout, "Panel %p created\n", this);
		this->z_rotation = 0.0;
		scale[0] = 1;
		scale[1] = 1;
		this->z_depth = 0;
		this->parent = parent;
		this->userdata = 0;
		this->visible = true;
		this->background_color = Color(255, 255, 255, 255);
		this->foreground_color = Color(0, 0, 0, 255);
		this->flags = (Flag_CursorEnabled | Flag_TransformIsDirty);
		
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
		for( ; it != end; ++it )
		{
			Panel * panel = (*it);
			panel->~Panel();
			gui::_gfree( panel );
		}

		children.clear();
		
		fprintf(stdout, "~Panel %p -> '%s'\n", this, "Unknown");
	} // ~Panel
	
	void Panel::set_bounds(const ScreenInt x, const ScreenInt y, const DimensionType width, const DimensionType height )
	{
		origin.x = x;
		origin.y = y;
		size.width = width;
		size.height = height;
	} // set_bounds
	
	const Rect& Panel::get_bounds() const
	{
		return bounds;
	} // get_bounds
	
	void Panel::get_screen_bounds(Rect& bounds)
	{
		bounds = this->bounds;
	} // get_screen_bounds
	
	void Panel::calculate_screen_bounds(Compositor* compositor)
	{
		Panel* parent = this->parent;
		
		ScreenInt width = compositor->width;
		ScreenInt height = compositor->height;
		
		Point origin = this->origin;
		Size size = this->size;
		
		// use the parent size as the basis for scaling
		if (parent != 0 && parent != compositor)
		{
			Size parent_size = parent->bounds.size;
			width = parent_size.width;
			height = parent_size.height;
			
			// add the origin offset of the parent
			origin = origin + parent->bounds.origin;
		}
		
		// TODO: modify offsets for anchors?
		
		bounds.size = size;
		bounds.origin = origin;
		
		if ( parent )
		{
			// add margins
		}
	} // calculate_screen_bounds
	
	void Panel::add_child( Panel * panel )
	{
		children.push_back( panel );
	} // add_child
	
	void Panel::remove_child( Panel * panel )
	{
		PanelVector::iterator it, end;
		it = children.begin();
		end = children.end();
		for( ; it != end; ++it )
		{
			if ( (*it) == panel )
			{
				children.erase( it );
				break;
			}
		}
	} // remove_child
	

	void Panel::handle_event( EventArgs & args )
	{
		if ( args.type == Event_CursorDrag )
		{
			if (can_move())
			{
				origin.x += args.delta.x;
				origin.y += args.delta.y;
			}
		}
		else if ( args.type == Event_CursorButtonReleased )
		{
			if ( args.cursor_button == gui::CursorButton::Middle )
			{
				fprintf(stdout, "bounds = {%2.2f, %2.2f, %g, %g}\n", bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
			}
		}
	} // handle_event

	void Panel::update(Compositor* compositor, const TimeState& timestate)
	{
		this->calculate_screen_bounds(compositor);
		
		// the points have to be rotated around the center pivot
		gui::Size sz = bounds.size;
		Point center(bounds.width()/2, bounds.height()/2);
		
		// center in local space
		center += bounds.origin;

		// calculate the bounds (local coordinates)
		geometry[0] = bounds.origin;
		geometry[1] = bounds.origin + Point(0, sz.height);
		geometry[2] = bounds.origin + Point(sz.width, sz.height);
		geometry[3] = bounds.origin + Point(sz.width, 0);

		// transform to the origin
		geometry[0] -= center;
		geometry[1] -= center;
		geometry[2] -= center;
		geometry[3] -= center;

		// update the local_transform matrix
		if (flags & Flag_TransformIsDirty)
		{
			flags &= ~Flag_TransformIsDirty;
			local_transform = rotation_matrix(z_rotation) * scale_matrix(scale);
		}

		transform_geometry(geometry, 4, local_transform);

		// transform back to the panel's position
		geometry[0] += center;
		geometry[1] += center;
		geometry[2] += center;
		geometry[3] += center;

		for(PanelVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			(*it)->update(compositor, timestate);
		}
	} // update

	void Panel::render(Rect& frame, Compositor* compositor, Renderer* renderer)
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
				gui::Color(255, 255, 255, 255)
			);
		}

		for(PanelVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			Panel* child = (*it);
			if (child->is_visible())
			{
				child->begin_render_frame(compositor);
				child->render(child->bounds, compositor, renderer);
				child->end_render_frame(compositor);
			}
		}
	} // render
	
	void Panel::set_background_image(Compositor* compositor, const char* path)
	{
		// This needs to be updated to use the new ResourceCache
		assert(0);
//		if (compositor && compositor->get_renderer())
//		{
//			compositor->renderer->texture_create(path, background);
//		}
	} // set_background_image
	
	
	void Panel::set_background_color(const Color& color)
	{
		background_color = color;
	}
	
	void Panel::set_foreground_color(const Color& color)
	{
		foreground_color = color;
	}
	
	void Panel::set_userdata(void* data)
	{
		userdata = data;
	} // set_userdata
	
	void* Panel::get_userdata() const
	{
		return userdata;
	} // get_userdata
	
	void Panel::set_visible(bool is_visible)
	{
		this->visible = is_visible;
	} // set_visible
	
	bool Panel::is_visible() const
	{
		return this->visible;
	} // is_visible
	
//	Point Panel::local_to_world(const Point& local) const
//	{
//		Point world;
//		world = origin + local;
//		return world;
//	} // local_to_world
//	
//	Point Panel::world_to_local(const Point& world) const
//	{
//		Point local;
//		local = world - origin;
//		return local;
//	} // world_to_local

	bool Panel::hit_test_local(const Point& local_point) const
	{
		return bounds.is_point_inside(local_point);
	}

	// ---------------------------------------------------------------------
	// transforms
	// ---------------------------------------------------------------------
	void Panel::set_rotation(const float radians)
	{
		z_rotation = radians;
		flags |= Flag_TransformIsDirty;
	}

	void Panel::set_scale(const glm::vec2& scale)
	{
		this->scale = scale;
		flags |= Flag_TransformIsDirty;
	}

	// ---------------------------------------------------------------------
	// render utils
	// ---------------------------------------------------------------------
	void Panel::begin_render_frame(Compositor* compositor)
	{
		render_commands.begin(compositor);
	}

	void Panel::end_render_frame(Compositor* compositor)
	{
		render_commands.end(compositor);
	}

	// ---------------------------------------------------------------------
	// other utils
	// ---------------------------------------------------------------------
	Compositor* Panel::get_compositor()
	{
		Panel* panel = parent;
		while(panel->parent)
			panel = panel->parent;

		assert(panel);
		return static_cast<Compositor*>(panel);
	}

} // namespace gui

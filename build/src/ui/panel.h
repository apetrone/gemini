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

#include "ui/ui.h"
#include "ui/utils.h"

#include <core/stackstring.h>

#include <stdint.h>
#include <new> // for size_t
#include <stddef.h>
#include <vector>

namespace gui
{
	class Compositor;
	class Renderer;
	class EventArgs;
	class Panel;

	typedef std::vector<Panel*> PanelVector;

	class Panel
	{
	public:
		// memory overrides
		void* operator new(size_t bytes);
		void operator delete(void* memory);
		
	public:
	
		enum PanelFlags
		{
			Flag_None,
			Flag_CursorEnabled		= 1, // if this can receive cursor input
			Flag_IsVisible			= 2,
			Flag_CanMove			= 4,

			Flag_TransformIsDirty	= 16
		};

		size_t z_depth;
		TextureHandle background;
		Panel* parent;
		PanelVector children;
		void* userdata;
		bool visible;
		Color background_color;
		Color foreground_color;
		
		uint32_t flags;
		
		LIBRARY_EXPORT Panel(Panel* parent);
		LIBRARY_EXPORT virtual ~Panel();

		LIBRARY_EXPORT virtual void set_bounds(const ScreenInt x, const ScreenInt y, const DimensionType width, const DimensionType height);
		LIBRARY_EXPORT virtual void set_bounds(const Rect& bounds);

		LIBRARY_EXPORT virtual void set_dimensions(float x, float y);

		LIBRARY_EXPORT virtual void get_screen_bounds(Rect& bounds);
		LIBRARY_EXPORT virtual void calculate_screen_bounds(Compositor* compositor);
		LIBRARY_EXPORT virtual void add_child(Panel* panel);
		LIBRARY_EXPORT virtual void remove_child(Panel* panel);
		LIBRARY_EXPORT virtual void handle_event(EventArgs& args);
		LIBRARY_EXPORT virtual void update(Compositor* compositor, float delta_seconds);
		LIBRARY_EXPORT virtual void render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands);
		LIBRARY_EXPORT virtual void render_children(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands);
		LIBRARY_EXPORT virtual void set_background_image(Compositor* compositor, const char* path);
		LIBRARY_EXPORT virtual void set_background_color(const Color& color);
		LIBRARY_EXPORT virtual void set_foreground_color(const Color& color);
		LIBRARY_EXPORT virtual void set_visible(bool is_visible);
		LIBRARY_EXPORT virtual bool is_visible() const;

//		LIBRARY_EXPORT Point local_to_world(const Point& local) const;
//		LIBRARY_EXPORT Point world_to_local(const Point& world) const;

		// ---------------------------------------------------------------------
		// hit tests
		// ---------------------------------------------------------------------
		LIBRARY_EXPORT virtual bool hit_test_local(const Point& local_point) const;

		// determine if this panel can become the foreground window (if it can be the first Z-ordered window)
		LIBRARY_EXPORT virtual bool can_send_to_front() const { return false; }
		
		// use this until we get a better system inplace for type checks / registration
		LIBRARY_EXPORT virtual bool is_label() const { return false; }
		LIBRARY_EXPORT virtual bool is_button() const { return false; }
		
		LIBRARY_EXPORT virtual bool has_flags(const uint32_t& flags) const { return (this->flags & flags) == flags; }

		// ---------------------------------------------------------------------
		// transforms
		// ---------------------------------------------------------------------
		LIBRARY_EXPORT void set_rotation(const float radians);
		LIBRARY_EXPORT void set_scale(const glm::vec2& scale);

		// ---------------------------------------------------------------------
		// other utils
		// ---------------------------------------------------------------------
		// traverse the hierarchy and find the parent compositor
		LIBRARY_EXPORT Compositor* get_compositor();

		LIBRARY_EXPORT const Point& get_origin() const { return origin; }
		LIBRARY_EXPORT virtual void set_origin(float x, float y);

		LIBRARY_EXPORT const Size& get_size() const { return size; }
		LIBRARY_EXPORT void set_size(const Size& new_size) { size = new_size; }

		LIBRARY_EXPORT const char* get_name() { return debug_name(); }
		LIBRARY_EXPORT void set_name(const char* name) { debug_name = name; }

	protected:
		core::StackString<64> debug_name;

		// origin local to the parent
		Point origin;

		// size expressed in actual pixels (computed during an update)
		Size size;

		// normalized panel dimensions [0,1] as a percentage
		// of the parent's dimensions.
		Point dimensions;

		// screen-space (dynamically computed; transient) bounds
		Rect bounds;

		friend class Compositor;

		// Panels will transform through the following spaces:
		// Local (or bounds)
		// Compositor (essentially world-space)
		// Screen

		glm::mat2 local_transform;

		// local scaling factor applied on top of initial dimensions
		glm::vec2 scale;

		// local rotation applied to panel
		real z_rotation;

		glm::vec2 geometry[4];
	}; // class Panel
} // namespace gui

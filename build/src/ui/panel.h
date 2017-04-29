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
#include <core/typespec.h>

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
	class Layout;

	typedef std::vector<Panel*> PanelVector;

	class Panel
	{
		TYPESPEC_DECLARE_CLASS_NOBASE(Panel)

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
			Flag_CanMove			= 4, // Should this panel accept drag events?
			Flag_CanDrop			= 8, // Should this panel accept drop events?

			Flag_TransformIsDirty	= 16,
			Flag_NeedsLayout		= 32,
			Flag_AlwaysOnTop		= 64
		};

		size_t z_depth;
		TextureHandle background;
		Panel* parent;
		PanelVector children;
		PanelVector zsorted;
		void* userdata;
		bool visible;
		gemini::Color background_color;
		gemini::Color foreground_color;

		Panel(Panel* parent);
		virtual ~Panel();

		virtual void set_bounds(const ScreenInt x, const ScreenInt y, const ScreenInt width, const ScreenInt height);
		virtual void set_bounds(const Rect& bounds);

		// compute content bounds (may exceed compositor bounds)
		virtual void get_content_bounds(Rect& bounds) const;

		virtual void add_child(Panel* panel);
		virtual void remove_child(Panel* panel);
		virtual void handle_event(EventArgs& args);
		virtual void update(Compositor* compositor, float delta_seconds);
		virtual void render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands);
		virtual void render_children(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands);
		void render_geometry(gui::render::CommandList& render_commands, const gemini::Color& color);
		virtual void set_background_image(Compositor* compositor, const char* path);
		virtual void set_background_color(const gemini::Color& color);
		virtual void set_foreground_color(const gemini::Color& color);
		virtual void set_visible(bool is_visible);
		virtual bool is_visible() const;

		size_t total_children() const;
		Panel* child_at(size_t index);
		Panel* child_at(size_t index) const;

		void clear_children();

		// Return the minimum size of this panel
		virtual void measure(Size& size) const;

		// Sets the size of this panel to the requested_size.
		virtual void resize(const Size& requested_size);

		void set_maximum_size(const Size& max_size);
		const Size& get_maximum_size() const;

		void set_minimum_size(const Size& min_size);
		const Size& get_minimum_size() const;

		// bring this panel forward in its parent's rendering order.
		void bring_to_front();

		// ---------------------------------------------------------------------
		// hit tests
		// ---------------------------------------------------------------------
		virtual bool hit_test_local(const Point& local_point) const;

		// determine if this panel can become the foreground window (if it can be the first Z-ordered window)
		virtual bool can_send_to_front() const { return true; }

		// use this until we get a better system in place for type checks / registration
		virtual bool is_label() const { return false; }
		virtual bool is_button() const { return false; }

		virtual bool has_flags(const uint32_t& test_flags) const { return (flags & test_flags) == test_flags; }
		virtual void set_flags(uint32_t new_flags) { flags = new_flags; }
		virtual uint32_t get_flags() const { return flags; }

		// ---------------------------------------------------------------------
		// transforms
		// ---------------------------------------------------------------------
		void set_rotation(const float radians);
		void set_scale(const glm::vec2& scale);

		// ---------------------------------------------------------------------
		// other utils
		// ---------------------------------------------------------------------

		// convert compositor coordinates to local panel coordinates
		Point compositor_to_local(const Point& location);

		virtual bool point_in_capture_rect(const Point& local) const;

		void set_layout(Layout* layout_instance);
		void set_name(const char* name) { debug_name = name; }
		virtual void set_origin(float x, float y);
		void set_origin(const Point& origin);
		virtual void set_size(const Size& new_size);
		void set_size(uint32_t width, uint32_t height);

		void swap_child(size_t first_index, size_t second_index);

		// traverse the hierarchy and find the parent compositor
		Compositor* get_compositor();
		Layout* get_layout() const { return layout; }
		const char* get_name() const { return debug_name(); }
		const Point& get_origin() const { return origin; }
		Panel* get_parent() { return parent; }
		const Size& get_size() const { return size; }
		Size get_client_size() const {
			return Size(size.width, size.height - capture_rect.size.height);
		}
		glm::mat3 get_transform(size_t index) const;

	protected:

		void update_transform(Compositor*);
		void zsort_children(Panel* pane);

		uint32_t flags;


		core::StackString<64> debug_name;

		// origin local to the parent
		Point origin;

		// size expressed in actual pixels (computed during an update)
		Size size;

		// Maximum size for this panel.
		// 0 == no maximum. Can be a combination (0, 400) to limit in only
		// the vertical direction.
		Size maximum_size;

		Size minimum_size;

		friend class Compositor;

		// Panels will transform through the following spaces:
		// Local (or bounds)
		// Compositor (essentially world-space)
		// Screen

		// homogeneous coordinates for this panel's local transform
		// translation * pivot * rotation * scale * inverse_pivot
		glm::mat3 local_transform;

		// local scaling factor applied on top of initial dimensions
		glm::vec2 scale;

		// local rotation applied to panel (in radians)
		real z_rotation;

		glm::vec2 geometry[4];

		// 'Capture' rectangle tested when compositor needs to set
		// a capture on a panel. Might not need this...
		Rect capture_rect;

		Layout* layout;
	}; // class Panel
} // namespace gui

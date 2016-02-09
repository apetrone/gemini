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

#include "ui/panel.h"
#include "ui/events.h"
#include "ui/utils.h"
#include "ui/scrollable.h"

#include <core/typedefs.h>

#include <string>

namespace gui
{
	class Label : public ScrollablePanel
	{
	public:
		LIBRARY_EXPORT Label(Panel* parent);

		LIBRARY_EXPORT virtual void get_content_bounds(Rect& bounds) const;
		LIBRARY_EXPORT virtual void update(Compositor* compositor, float delta_seconds) override;
		LIBRARY_EXPORT virtual void render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands) override;
		LIBRARY_EXPORT virtual void set_font(const char* filename, size_t pixel_size);
		LIBRARY_EXPORT virtual void set_text(const std::string& text);
		LIBRARY_EXPORT virtual bool is_label() const override { return true; }

		LIBRARY_EXPORT void append_text(const char* message);

	protected:

		// this should be called whenever the text is modified
		// to recalculate the content rect.
		void update_text_cache();

		struct font_cache_entry
		{
			glm::vec2 origin;
			size_t start;
			size_t length;
		};

		std::string text;
		FontHandle font_handle;
		Point text_origin;
		Rect content_bounds;

		// font cache stuff
		Array<font_cache_entry> font_cache;
		size_t font_cache_index;
		uint32_t cache_is_dirty;
		uint32_t font_height;
	}; // Label
} // namespace gui

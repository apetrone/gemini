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
#include "ui/scrollbar.h"

#include <core/typedefs.h>
#include <core/logging.h>
#include <core/typespec.h>

#include <string>

namespace gui
{
	class ScrollablePanel : public gui::Panel
	{
		TYPESPEC_DECLARE_CLASS(ScrollablePanel, Panel);

	public:
		ScrollablePanel(gui::Panel* parent);
		virtual void update(gui::Compositor* compositor, float delta_seconds) override;
		virtual void handle_event(gui::EventArgs& args);

		void scroll_to_bottom();

	protected:
		void update_scrollbars();

		void on_vertical_scroll(float value);
		void on_horizontal_scroll(float value);

		Scrollbar* horizontal_bar;
		Scrollbar* vertical_bar;
		Point scroll_offset;
	}; // ScrollablePanel
} // namespace gui

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

#include "ui/button.h"
#include "ui/panel.h"
#include "ui/events.h"
#include "ui/utils.h"

#include <core/typedefs.h>

#include <string>

namespace gui
{
	class ScrollButton : public Button
	{
	public:
		ScrollButton(Panel* parent, uint32_t direction = 0);

	protected:
		// 0 for horizontal
		// 1 for vertical
		uint32_t scroll_direction;

		// value is normalized [0, 1]
		float scroll_value;
	};

	class Scrollbar : public Panel
	{
	public:
		Scrollbar(Panel* parent, uint32_t direction);

		virtual void handle_event(EventArgs& args) override;

		// set the button dimensions on this scrollbar
		// can be used by ScrollablePanels to set the side of the buttons
		// depending on the content size.
		void set_button_dimensions(float x, float y);

		void set_scroll_value(float new_value);

		gemini::DelegateHandler<float> on_scroll_value_changed;

	protected:

		ScrollButton* bootun;
		bool is_dragging;

		Point initial_click;
		uint32_t deferred_flags;
	};
} // namespace gui

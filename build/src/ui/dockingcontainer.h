// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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
//#include "ui/utils.h"
//#include "ui/scrollable.h"

#include <core/typedefs.h>

// The design is inspired in part by:
// http://www.codeproject.com/Articles/140209/Building-a-Docking-Window-Management-Solution-in-W
// Most importantly, the different states of the main docking container
// and the multiple behaviors afforded to the user.

// Rules observed from Visual Studio's own UI:

// Dockable containers typically have four Dockable Panel-style windows
// (one in each cardinal direction) and a single central, Document window.

// Each Dockable panel can be stacked with tabs to organize multiple panels
// in the same location.
// Dockable panel tabs can be re-ordered.
// Dockable panels can float.

// The document style panel resides in the center and
// offers an additional ability: to split the current document window with a
// newly docked panel.

// Dockable Documents can ONLY be docked/split inside the central document
// panel.

// DockablePanels can be docked/split inside the central panel, as well as
// attached at the bottom of central panels, and also be docked to a cardinal
// side of the top level docking container.

#if 0
https://www.youtube.com/user/handmadeheroarchive/videos
  http://www.codeproject.com/Articles/140209/Building-a-Docking-Window-Management-Solution-in-W
#endif

namespace gui
{
	class DockingContainer : public Panel
	{
	private:
		// left, right, top, bottom, center
		gui::Rect regions[5];
		gemini::Color colors[5];


	public:
		DockingContainer(Panel* parent);

		virtual void handle_event(EventArgs& args) override;
		virtual void update(Compositor* compositor, float delta_seconds) override;
		virtual void render(Compositor* compositor, Renderer* renderer, gui::render::CommandList& render_commands) override;
	protected:
	}; // DockingContainer
} // namespace gui

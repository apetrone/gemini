// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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

#include <gui/render_panel.h>
#include <ui/compositor.h>
#include <renderer/renderer.h>

namespace gui
{
	void RenderableSurface::render(gui::Compositor* /*compositor*/, gui::Renderer* /*renderer*/, gui::render::CommandList& render_commands)
	{
		gui::Painter painter(this, render_commands);

		if (on_render_content.is_valid())
		{
			on_render_content(target);
		}

		painter.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], handle, gemini::Color::from_rgba(255, 255, 255, 255));
	}

	void RenderableSurface::set_size(const Size& new_size)
	{
		// resize the render target
		LOGV("resize render target %i %i\n", new_size.width, new_size.height);
		gui::Panel::set_size(new_size);
		//https://blog.nelhage.com/2016/12/how-i-test/
		render_device->resize_render_target(target, new_size.width, new_size.height);
	}
} // namespace gui
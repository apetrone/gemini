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

#include "spring_panel.h"

namespace gemini
{
	void SpringPanel::update(gui::Compositor* compositor, float delta_seconds)
	{
		gui::Panel::update(compositor, delta_seconds);
#if 0
		// Simple harmonic oscillator
		x = box.position.x - target.x;
		k = 15.f;

		const float mass_kgs = .045f;
		//float w = sqrt(k / m);
		//const float frequency = (w / (2 * mathlib::PI));
		//const float period = 1.0f / frequency;
		const float T = (mathlib::PI * 2.0) * (sqrt(mass_kgs / k));
		const float frequency = (1.0f / T);
		LOGV("freq: %2.2fHz\n", frequency);


		box.velocity.x += -k*x * delta_seconds;
		box.position += box.velocity * delta_seconds;
#endif

#if 1
		// Damped harmonic oscillator
		x = box.position.x - target.x;
		k = 0.125f;
		const float mass_kgs = .045f;
		const float c = 0.19f;

		//const float damping_ratio = (c / (2.0 * sqrt(mass_kgs * k)));
		//LOGV("damping_ratio is %2.2f\n", damping_ratio);

		//box.velocity += 0.75f * (-(box.position - target) * delta_seconds);
		box.velocity.x += -k*x - c * box.velocity.x;
		box.position += box.velocity;
#endif
	}

	void SpringPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
	{
		gui::Painter painter(this, render_commands);

		gui::Panel::render(compositor, renderer, render_commands);

		painter.add_rectangle(
			box.position + tube[0],
			box.position + tube[1],
			box.position + tube[2],
			box.position + tube[3],
			gui::render::WhiteTexture,
			gemini::Color(1.0f, 0.5f, 0.0)
		);
	}
} // namespace gemini
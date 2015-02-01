// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone
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

#include <string>

#include <core/mathlib.h>
#include "color.h"

#include <sdk/debugdraw_api.h>

namespace gemini
{
	namespace renderer
	{
		struct Font;
		struct ShaderProgram;
	}

	namespace debugdraw
	{
		void startup(unsigned int max_primitives, renderer::ShaderProgram* program, const renderer::Font& font);
		void shutdown();
		
		void update(float deltamsec);
		void render(const glm::mat4& modelview, const glm::mat4& projection, int x, int y, int viewport_width, int viewport_height);
		
		void axes(const glm::mat4& transform, float axis_length, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void basis(const glm::vec3& origin, const glm::vec3& basis, float axis_length, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void box(const glm::vec3& mins, const glm::vec3& maxs, const core::Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void point(const glm::vec3& pt, const core::Color& color, float size = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void line(const glm::vec3& start, const glm::vec3& end, const core::Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void sphere(const glm::vec3& center, const core::Color& color, float radius = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void text(int x, int y, const char* string, const core::Color& color, float duration = 0.0f);

	} // debugdraw
} // namespace gemini
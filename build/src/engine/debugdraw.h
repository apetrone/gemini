// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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
		void box(const glm::vec3& mins, const glm::vec3& maxs, const Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void point(const glm::vec3& pt, const Color& color, float size = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void line(const glm::vec3& start, const glm::vec3& end, const Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void sphere(const glm::vec3& center, const Color& color, float radius = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		void text(int x, int y, const char* string, const Color& color, float duration = 0.0f);

	} // debugdraw
} // namespace gemini
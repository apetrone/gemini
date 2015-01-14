// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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

#include <core/interface.h>
#include <core/color.h>

namespace gemini
{
	const float DEBUGDRAW_MIN_DURATION_MSEC = 0.1;

	class IDebugDraw
	{
	public:
		virtual ~IDebugDraw() {};

		virtual void axes(const glm::mat4& transform, float length, float duration = DEBUGDRAW_MIN_DURATION_MSEC) = 0;
		virtual void box(const glm::vec3& mins, const glm::vec3& maxs, const Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC) = 0;
		virtual void point(const glm::vec3& pt, const Color& color, float size = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC) = 0;
		virtual void line(const glm::vec3& start, const glm::vec3& end, const Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC) = 0;
		virtual void sphere(const glm::vec3& center, const Color& color, float radius = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC) = 0;
		virtual void text(int x, int y, const char* string, const Color& color, float duration = 0) = 0;
	};
	
	namespace debugdraw
	{
		static Interface<IDebugDraw> instance;
	} // namespace debugdraw
	
} // namespace gemini
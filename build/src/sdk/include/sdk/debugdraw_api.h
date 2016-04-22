// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <core/typedefs.h>
#include <core/interface.h>

#include <renderer/color.h>

namespace gemini
{
	class IDebugDraw
	{
	public:
		virtual ~IDebugDraw();

		// draw axes for a matrix4
		virtual void axes(const glm::mat4& transform,
			float axis_length,
			float duration = 0.0f) = 0;

		// draw basis vectors at origin, assuming unit length basis
		virtual void basis(const glm::vec3& origin,
			const glm::vec3& basis,
			float axis_length,
			float duration = 0.0f) = 0;

		// draw a box given the mins and maxs
		virtual void box(const glm::vec3& mins,
			const glm::vec3& maxs,
			const Color& color,
			float duration = 0.0f) = 0;

		// draw a point in space
		virtual void point(const glm::vec3& pt,
			const Color& color,
			float size = 2.0f,
			float duration = 0.0f) = 0;

		// draw a line segment
		virtual void line(const glm::vec3& start,
			const glm::vec3& end,
			const Color& color,
			float duration = 0.0f) = 0;

		// draw a sphere
		virtual void sphere(const glm::vec3& center,
			const Color& color,
			float radius = 2.0,
			float duration = 0.0f) = 0;

		// draw text; origin is at the baseline (lower left)
		// in screen space coordinates.
		virtual void text(int x,
			int y,
			const char* string,
			const Color& color,
			float duration = 0.0f) = 0;

		// draw a triangle
		virtual void triangle(const glm::vec3& v0,
			const glm::vec3& v1,
			const glm::vec3& v2,
			const Color& color,
			float duration = 0.0f) = 0;

		// draw camera
		virtual void camera(const glm::vec3& origin,
			const glm::vec3& view,
			float duration = 0.0f) = 0;
	};

	namespace debugdraw
	{
		GEMINI_DECLARE_INTERFACE(IDebugDraw);
	} // namespace debugdraw

} // namespace gemini

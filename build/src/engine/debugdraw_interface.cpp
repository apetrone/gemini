// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include "debugdraw_interface.h"
#include <renderer/debug_draw.h>

namespace gemini
{
	IDebugDraw::~IDebugDraw()
	{
	}
	
	void DebugDrawInterface::axes(const glm::mat4& transform, float axis_length, float duration)
	{
		debugdraw::axes(transform, axis_length, duration);
	} // axes
	
	void DebugDrawInterface::basis(const glm::vec3& origin, const glm::vec3& basis, float axis_length, float duration)
	{
		debugdraw::basis(origin, basis, axis_length, duration);
	} // basis
	
	void DebugDrawInterface::box(const glm::vec3& mins, const glm::vec3& maxs, const core::Color& color, float duration)
	{
		debugdraw::box(mins, maxs, color, duration);
	} // box
	
	void DebugDrawInterface::point(const glm::vec3& pt, const core::Color& color, float size, float duration)
	{
		debugdraw::point(pt, color, size, duration);
	} // point
	
	void DebugDrawInterface::line(const glm::vec3& start, const glm::vec3& end, const core::Color& color, float duration)
	{
		debugdraw::line(start, end, color, duration);
	} // line
	
	void DebugDrawInterface::sphere(const glm::vec3& center, const core::Color& color, float radius, float duration)
	{
		debugdraw::sphere(center, color, radius, duration);
	} // sphere
		
	void DebugDrawInterface::text(int x, int y, const char* string, const core::Color& color, float duration)
	{
		debugdraw::text(x, y, string, color, duration);
	} // text
	
	void DebugDrawInterface::triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const core::Color& color, float duration)
	{
		debugdraw::triangle(v0, v1, v2, color, duration);
	} // triangle
} // namespace gemini
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
#include "debugdraw.h"
#include "debugdraw_interface.h"

#include <sdk/debugdraw_api.h>

namespace gemini
{
	namespace debugdraw
	{
		void startup(unsigned int max_primitives, renderer::ShaderProgram* program, const renderer::Font& font)
		{
			// create an instance for the api
			DebugDrawInterface* debugdraw_instance = CREATE(DebugDrawInterface);
			debugdraw::instance = debugdraw_instance;
			debugdraw_instance->startup(max_primitives, program, font);
		} // startup
		
		
		void shutdown()
		{
			// destroy the instance for the api
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->shutdown();
			
			IDebugDraw* debugdraw_instance = debugdraw::instance();
			DESTROY(IDebugDraw, debugdraw_instance);
			debugdraw::instance = 0;
		} // shutdown
		
		
		void update(float delta_msec)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->update(delta_msec);
		} // void update
		
		
		void render(const glm::mat4& modelview, const glm::mat4& projection, int x, int y, int viewport_width, int viewport_height)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->render(modelview, projection, x, y, viewport_width, viewport_height);
		} // render
		
		
		void axes(const glm::mat4& transform, float axis_length, float duration)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->axes(transform, axis_length, duration);
		} // axes
		
		void basis(const glm::vec3& origin, const glm::vec3& basis, float axis_length, float duration)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->basis(origin, basis, axis_length, duration);
		} // basis
		
		void box(const glm::vec3& mins, const glm::vec3& maxs, const Color& color, float duration)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->box(mins, maxs, color, duration);
		} // box
		
		
		void point(const glm::vec3& pt, const Color& color, float size, float duration)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->point(pt, color, size, duration);
		} // point
		
		
		void line(const glm::vec3& start, const glm::vec3& end, const Color& color, float duration)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->line(start, end, color, duration);
		} // line
		
		
		void sphere(const glm::vec3& center, const Color& color, float radius, float duration)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->sphere(center, color, radius, duration);
		} // sphere


		void text(int x, int y, const char* string, const Color& color, float duration)
		{
			DebugDrawInterface* debugdraw_interface = static_cast<DebugDrawInterface*>(debugdraw::instance());
			debugdraw_interface->text(x, y, string, color, duration);
		} // text
	} // namespace debugdraw
} // namespace gemini
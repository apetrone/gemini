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
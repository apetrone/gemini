// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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

#include <sdk/debugdraw_api.h>

#include <string>

#include <renderer/shaderprogram.h>
#include <renderer/vertexstream.h>
#include <renderer/font.h>

namespace gemini
{
	namespace debugdraw
	{
		enum
		{
			TYPE_BOX=1,
			TYPE_LINE,
			TYPE_AXES,
			TYPE_SPHERE,
			TYPE_TEXT,
			
			TYPE_COUNT
		};
		
		struct DebugPrimitive
		{
			short type; // type of primitive
			short flags;
			float timeleft; // the time left until this object is no longer rendered
			float radius;
			glm::vec3 start;
			glm::vec3 end;
			Color color;
			
			// for text
			std::string buffer;
			glm::mat4 transform;
			
			DebugPrimitive();
		}; // DebugPrimitive
		
		struct DebugDrawVertex
		{
			glm::vec3 position;
			Color color;
		}; // DebugDrawVertex
	} // namespace debugdraw

	class DebugDrawInterface : public gemini::IDebugDraw
	{
		renderer::VertexStream vertex_stream;
		unsigned int next_primitive;
		unsigned int max_primitives;
		debugdraw::DebugPrimitive* primitive_list;
		renderer::Font debug_font;
		renderer::ShaderProgram* debug_shader;
	private:
	
		debugdraw::DebugPrimitive* request_primitive();
	
	public:
		void startup(unsigned int max_primitives, renderer::ShaderProgram* program, const renderer::Font& font);
		void shutdown();
		void update(float delta_msec);
		
		void render(const glm::mat4 & modelview, const glm::mat4 & projection, int x, int y, int viewport_width, int viewport_height);
		void generate_circle(const glm::vec3 & origin, glm::vec3 * vertices, int num_sides, float radius, int plane);
		
		renderer::Font get_debug_font() const { return debug_font; }
		
		// IDebugDraw interface
		virtual void axes(const glm::mat4& transform, float axis_length, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void box(const glm::vec3& mins, const glm::vec3& maxs, const Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void point(const glm::vec3& pt, const Color& color, float size = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void line(const glm::vec3& start, const glm::vec3& end, const Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void sphere(const glm::vec3& center, const Color& color, float radius = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void text(int x, int y, const char* string, const Color& color, float duration = 0);
	}; // DebugDrawInterface
	
} // namespace gemini
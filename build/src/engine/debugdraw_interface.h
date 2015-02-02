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
		// if you modify this, you must also update the buffer_primitive_table.
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
			core::Color color;
			
			// for text
			std::string buffer;
			glm::mat4 transform;
			
			DebugPrimitive();
		}; // DebugPrimitive
		
		struct DebugDrawVertex
		{
			glm::vec3 position;
			core::Color color;
		}; // DebugDrawVertex
	} // namespace debugdraw

	class DebugDrawInterface : public gemini::IDebugDraw
	{
		renderer::VertexStream vertex_stream;
		unsigned int next_primitive;
		unsigned int max_primitives;
		debugdraw::DebugPrimitive* primitive_list;
		font::Handle debug_font;
		renderer::ShaderProgram* debug_shader;
	private:
	
		debugdraw::DebugPrimitive* request_primitive();
	
	public:
		void startup(unsigned int max_primitives, renderer::ShaderProgram* program, const font::Handle& font);
		void shutdown();
		void update(float delta_msec);
		
		void render(const glm::mat4 & modelview, const glm::mat4 & projection, int x, int y, int viewport_width, int viewport_height);
		void generate_circle(const glm::vec3 & origin, glm::vec3 * vertices, int num_sides, float radius, int plane);
		
		font::Handle get_debug_font() const { return debug_font; }
		
		// IDebugDraw interface
		virtual void axes(const glm::mat4& transform, float axis_length, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void basis(const glm::vec3& origin, const glm::vec3& basis, float axis_length, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void box(const glm::vec3& mins, const glm::vec3& maxs, const core::Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void point(const glm::vec3& pt, const core::Color& color, float size = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void line(const glm::vec3& start, const glm::vec3& end, const core::Color& color, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void sphere(const glm::vec3& center, const core::Color& color, float radius = 2.0, float duration = DEBUGDRAW_MIN_DURATION_MSEC);
		virtual void text(int x, int y, const char* string, const core::Color& color, float duration = 0);
	}; // DebugDrawInterface
	
} // namespace gemini
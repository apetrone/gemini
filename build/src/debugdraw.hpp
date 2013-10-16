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

#include "mathlib.h"
#include "color.hpp"

namespace debugdraw
{
	const float MIN_DURATION_MSEC = 0.1;
	const char DEBUG_FONT_FILE[] = "fonts/debug";
	
	const int MAX_CIRCLE_SIDES = 12;
	const int TOTAL_CIRCLE_VERTICES = 2 * MAX_CIRCLE_SIDES;
	enum
	{
		TYPE_BOX=1,
		TYPE_LINE,
		TYPE_AXES,
		TYPE_SPHERE,
		
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
		
		DebugPrimitive();
	}; // DebugPrimitive
	
	struct DebugDrawVertex
	{
		glm::vec3 position;
		Color color;
	}; // DebugDrawVertex
	
	void startup( unsigned int max_primitives );
	void shutdown();
	
	void update( float deltamsec );
	void render( const glm::mat4 & modelview, const glm::mat4 & projection, int viewport_width, int viewport_height );
	
	void axes( const glm::mat4 & transform, float length, float duration = MIN_DURATION_MSEC );
	void box( const glm::vec3 & mins, const glm::vec3 & maxs, const Color & color, float duration = MIN_DURATION_MSEC );
	void point( const glm::vec3 & pt, const Color & color, float size = 2.0, float duration = MIN_DURATION_MSEC );
	void line( const glm::vec3 & start, const glm::vec3 & end, const Color & color, float duration = MIN_DURATION_MSEC );
	void sphere( const glm::vec3 & center, const Color & color, float radius = 2.0, float duration = MIN_DURATION_MSEC );
	void text( int x, int y, const char * string, const Color & color );

}; // debugdraw

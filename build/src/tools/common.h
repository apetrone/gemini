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

#include <string>

#include <core/mathlib.h>

namespace tools
{
	struct IndentState
	{
		size_t depth;
		std::string buffer;
		
		IndentState() : depth(0)
		{}
		
		void push()
		{
			++depth;
			update();
		}
		
		void pop()
		{
			--depth;
			update();
		}
		
		void update()
		{
			buffer.clear();
			for (size_t i = 0; i < depth; ++i)
			{
				buffer += "    ";
			}
		}
		
		const char* indent()
		{
			return buffer.c_str();
		}
	};
		
	void startup();
	void shutdown();
}

namespace datamodel
{
	struct Mesh;
}

namespace geometry
{
	glm::vec3 compute_center_of_mass(struct datamodel::Mesh* mesh);
}
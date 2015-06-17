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

#include <core/mathlib.h>
#include <core/str.h>

#include <string>

namespace gemini
{
	namespace tools
	{
		struct IndentState
		{
			size_t depth;
			String buffer;
			
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
			
		void startup(const char* application_name);
		void shutdown();
	} // namespace tools

	namespace datamodel
	{
		struct Mesh;
	} // namespace datamodel

	namespace geometry
	{
		glm::vec3 compute_center_of_mass(struct datamodel::Mesh* mesh);
	} // namespace geometry
} // namespace gemini
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

#include <platform/typedefs.h>
#include <platform/mem.h>
#include <core/mathlib.h>
#include <core/str.h>

#include <vector>

namespace gemini
{
	namespace datamodel
	{
		struct Mesh;
		struct Skeleton;

		typedef std::vector<struct Node*, CustomPlatformAllocator<struct Node*>> NodeVector;

		
		struct Node
		{
			enum Flags
			{
				None,
				HasAnimations, // this node has animations
			};
			
			uint32_t flags;
			
			String name;
			String type;
			
			// local transform
			glm::vec3 scale;
			
			// euler angles (yaw, pitch, roll)
			glm::quat rotation;
			glm::vec3 translation;
			
			// global transform
			glm::mat4 absolute_world_transform;

			Node* parent;
			NodeVector children;
			
			Mesh* mesh;
			
			Node();
			virtual ~Node();
			void add_child(Node* child);
			void remove_child(Node* child);
			Node* find_child_named(const String& name);
			
			bool has_animations() const { return (flags & HasAnimations) > 0; }
		};
	} // namespace datamodel
} // namespace gemini
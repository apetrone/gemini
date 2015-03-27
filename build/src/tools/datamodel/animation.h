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
#include <core/mathlib.h>
#include <core/str.h>

#include <vector>
#include <map>
#include <string>

//#include "datamodel/node.h"

namespace gemini
{
	namespace datamodel
	{
		template <class Type>
		struct Keyframe
		{
			float time_seconds;
			Type value;
			
			
			Keyframe(float seconds, Type val) : time_seconds(seconds), value(val)
			{
			}
		};
		
		template <class Type>
		struct Channel
		{
			typedef Keyframe<Type> KeyframeType;
			std::vector< KeyframeType* > keys;
			KeyframeType* add_key(float seconds, const Type& val)
			{
				KeyframeType* key = CREATE(KeyframeType, seconds, val);
				keys.push_back(key);
				return key;
			}
			
			~Channel()
			{
				for (auto key : keys)
				{
					DESTROY(KeyframeType, key);
				}
			}
		};

		struct NodeAnimation
		{
			// the name of the node corresponding to this data
			String name;
			
			Channel<glm::vec3> scale;
			Channel<glm::vec3> rotation;
			Channel<glm::vec3> translation;
		};

		// This represents a single animation
		struct Animation
		{
			typedef std::map<String, NodeAnimation*> NodeAnimationByNameContainer;
			
			// keyframe/node data
			std::vector< NodeAnimation* > node_animations;
			NodeAnimationByNameContainer nodes_by_name;
			
			// this animations' data
			String name;
			uint16_t frames_per_second;
			
			// total animation length in seconds
			float duration_seconds;
			
			Animation();
			~Animation();
			
			NodeAnimation* data_with_name(const String& node_name);
			NodeAnimation* add_node_data(const String& node_name);
		};
	} // namespace datamodel
} // namespace gemini
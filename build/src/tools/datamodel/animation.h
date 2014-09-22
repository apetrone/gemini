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

#include <gemini/typedefs.h>
#include <gemini/mathlib.h>

#include <vector>
#include <map>
#include <string>

//#include "datamodel/node.h"

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
		std::string name;
		
		Channel<glm::vec3> scale;
		Channel<glm::quat> rotation;
		Channel<glm::vec3> translation;
	};

	// This represents a single animation
	struct Animation
	{
		typedef std::map<std::string, NodeAnimation*> NodeAnimationByNameContainer;
		
		// keyframe/node data
		std::vector< NodeAnimation* > node_animations;
		NodeAnimationByNameContainer nodes_by_name;
		
		// this animations' data
		std::string name;
		uint16_t frames_per_second;
		
		Animation();
		~Animation();
		
		NodeAnimation* data_with_name(const std::string& node_name);
		NodeAnimation* add_node_data(const std::string& node_name);
	};
};
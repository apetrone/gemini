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

#include "datamodel/animation.h"

namespace gemini
{
	namespace datamodel
	{
		Animation::Animation() :
			name("Unnamed Animation"),
			frames_per_second(30)
		{
		}

		Animation::~Animation()
		{
			for (auto data : node_animations)
			{
				MEMORY_DELETE(data, core::memory::global_allocator());
			}
		}

		NodeAnimation* Animation::data_with_name(const String& node_name)
		{
			auto it = nodes_by_name.find(node_name);
			if (it != nodes_by_name.end())
			{
				return it->second;
			}

			return nullptr;
		}

		NodeAnimation* Animation::add_node_data(const String& node_name)
		{
			NodeAnimation* data = MEMORY_NEW(NodeAnimation, core::memory::global_allocator());
			data->name = node_name;
			node_animations.push_back(data);
			nodes_by_name.insert(NodeAnimationByNameContainer::value_type(node_name, data));

			return data;
		}
	} // namespace datamodel
} // namespace gemini

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

#include "datamodel/animation.h"

namespace datamodel
{
	Animation::Animation() :
		frames_per_second(30),
		name("Unnamed Animation")
	{
		
	}
	
	Animation::~Animation()
	{
		for (auto data : node_animations)
		{
			DESTROY(NodeAnimation, data);
		}
	}
	
	NodeAnimation* Animation::data_with_name(const std::string& node_name)
	{
		auto it = nodes_by_name.find(node_name);
		if (it != nodes_by_name.end())
		{
			return it->second;
		}
	
		return nullptr;
	}
	
	NodeAnimation* Animation::add_node_data(const std::string& node_name)
	{
		NodeAnimation* data = CREATE(NodeAnimation);
		data->name = node_name;
		node_animations.push_back(data);
		nodes_by_name.insert(NodeAnimationByNameContainer::value_type(node_name, data));
		
		return data;
	}
};
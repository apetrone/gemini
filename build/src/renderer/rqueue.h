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

#include "renderer.h"

#include <core/typedefs.h>
#include <platform/mem.h>

#include <vector>

namespace renderer
{
	typedef unsigned int RenderKey;

	// This is used for sorting the texture/shader combination
	struct RenderMaterial
	{
		uint16_t texture;
		uint16_t shader;
	};

	// Contains necessary data needed in order to submit this to the renderer
	typedef Geometry RenderObject;

	// An item in a queue/list that contains a sorting key and a pointer to a RenderObject
	// A RenderBlock is created for each renderable scene graph node.
	struct RenderBlock
	{
		RenderKey key;
		
		RenderObject* object;
		glm::mat4* object_matrix;
		
		unsigned int material_id;
		unsigned int shader_id;
		
		glm::mat4* node_transforms;
		uint32_t total_transforms;
		
		RenderBlock(RenderKey _key = 0, RenderObject* _object = 0) :
			key(_key),
			object(_object),
			object_matrix(0),
			
			material_id(0),
			shader_id(0),
			
			node_transforms(0),
			total_transforms(0)
		{
		}
			
		RenderBlock& operator= (const RenderBlock& other)
		{
			key = other.key;
			object = other.object;
			
			object_matrix = other.object_matrix;
			node_transforms = other.node_transforms;
			total_transforms = other.total_transforms;

			return *this;
		}
	};

	class RenderQueue
	{
	public:
		typedef std::vector< RenderBlock, CustomPlatformAllocator<RenderBlock> > RenderList;
		
		RenderList render_list;
	
		RenderQueue() {}
		~RenderQueue() {}

		void insert(const RenderBlock& block);
		void sort();
		void clear();
		size_t size() const;
	};
} // namespace renderer
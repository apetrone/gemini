// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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

#include <vector>

#include <gemini/typedefs.h>
#include <gemini/mem.h>


#include "renderer.h"

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
		
		RenderBlock(RenderKey _key, RenderObject* _object) :
			key(_key), object(_object) {}
	};



	class RenderQueue
	{
		typedef std::vector< RenderBlock*, GeminiAllocator<RenderBlock*> > RenderList;
		
		RenderList render_list;
		
		
	public:
		RenderQueue() {}
		~RenderQueue();
		
		void insert(RenderKey key, RenderObject* object);
		void sort();
		void draw();
		void purge();
		size_t size() const;
	};
}; // namespace renderer
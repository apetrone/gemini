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
#include <slim/xlog.h>

#include <algorithm>

#include "rqueue.h"

namespace renderer
{
	

	// this is a descending compare function used to traverse render commands
	// where higher keys have priority
	struct DescendingRenderBlockCompare
	{
		bool operator()(const RenderBlock* left, const RenderBlock* right)
		{
			if ( left->key < right->key )
			{
				return 1;
			}
			else if ( left->key > right->key )
			{
				return -1;
			}
			
			return 0;
		}
	};


	RenderQueue::~RenderQueue()
	{
		purge();
	}

	void RenderQueue::insert(RenderKey key, RenderObject *object)
	{
		RenderBlock* block = CREATE(RenderBlock, key, object);
		render_list.push_back(block);
	}

	void RenderQueue::sort()
	{
		std::sort(begin(render_list), end(render_list), DescendingRenderBlockCompare());
	}

	void RenderQueue::draw()
	{

	}

	void RenderQueue::purge()
	{
		std::for_each(begin(render_list), end(render_list), [](RenderBlock* block)
					  {
						  DESTROY(RenderBlock, block);
					  }
					  );
		
		render_list.clear();
	}

	size_t RenderQueue::size() const
	{
		return render_list.size();
	}
}; // namespace renderer
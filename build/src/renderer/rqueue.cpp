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
#include <core/logging.h>

#include <algorithm>

#include "rqueue.h"

namespace gemini
{
	namespace renderer
	{
		// this is a descending compare function used to traverse render commands
		// where higher keys have priority
		struct DescendingRenderBlockCompare
		{
			bool operator()(const RenderBlock& left, const RenderBlock& right)
			{
				if ( left.key < right.key )
				{
					return 1;
				}
				else if ( left.key > right.key )
				{
					return -1;
				}
				
				return 0;
			}
		};

		void RenderQueue::insert(const RenderBlock& block)
		{
			render_list.push_back(block);
		}

		void RenderQueue::sort()
		{
			std::sort(begin(render_list), end(render_list), DescendingRenderBlockCompare());
		}

		void RenderQueue::clear()
		{
			render_list.clear();
		}

		size_t RenderQueue::size() const
		{
			return render_list.size();
		}
	} // namespace renderer
} // namespace gemini
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


#include "scenelink.h"

namespace renderer
{
	namespace scenelink
	{
		static RenderQueue queue;
		
		class SceneVisitor : public scenegraph::Visitor
		{
			size_t total_nodes;
			
		public:
			virtual int visit(scenegraph::Node* node)
			{
				return 0;
			}
		};
		

		
		void add(scenegraph::Node* node)
		{
			RenderKey key = 0;
			RenderObject* object = 0;
			queue.insert(key, object);
		}
		

		void draw(scenegraph::Node* root)
		{
			// clear the queue and prepare for another frame
			queue.purge();
		
			// sort the queue
			queue.sort();
			
			// finally, draw from the queue
			queue.draw();
		}
	};
}; // namespace renderer
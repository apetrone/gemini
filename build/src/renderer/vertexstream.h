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
#include <gemini/typedefs.h>
#include "renderer.h"

namespace renderer
{
	// By default, this provides an array of Vertex data interleaved.
	// In order to use this data with separate data arrays,
	// 
	struct VertexStream
	{
		unsigned int _debug_flags;

		IndexType total_vertices;
		IndexType total_indices;
		IndexType last_vertex;
		IndexType last_index;
		IndexType highest_index;
		VertexType * vertices;
		IndexType * indices;
		unsigned int vertex_stride;
		
		VertexBuffer * vertexbuffer;

		VertexDescriptor desc;
		
		VertexStream();
		~VertexStream();
		void alloc( IndexType max_vertices, IndexType max_indices = 0 );
		void reset();
		void dealloc();
		unsigned int bytes_used();
		VertexType * request( IndexType num_vertices, int dont_advance_pointer = 0 );
		VertexType * operator[](int index);
		void append_indices( IndexType * inIndices, IndexType num_indices );
		bool has_room( unsigned int num_vertices, unsigned int num_indices ) const;
		
		void create( IndexType max_vertices, IndexType max_indices, renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type = renderer::BUFFER_STATIC );
		void destroy();
		void update();
		void draw_elements();
		void draw();
		void fill_data( VertexType * vertex_source, unsigned int vertex_count, IndexType * index_source, unsigned int index_count );
		
	}; // VertexStream

}; // namespace renderer

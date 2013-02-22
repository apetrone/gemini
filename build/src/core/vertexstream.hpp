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
#include "typedefs.h"
#include "renderer.hpp"

namespace renderer
{
	struct VertexStream
	{
		typedef unsigned char VertexType;

		unsigned int _debug_flags;

		IndexType totalVertices;
		IndexType totalIndices;
		IndexType lastVertex;
		IndexType lastIndex;
		IndexType highestIndex;
		VertexType * vertices;
		IndexType * indices;
		unsigned int vertexStride;
		
		VertexBuffer * vertexbuffer;
		
		// these are used for rendering
		unsigned int vao;
		unsigned int vbo[2];
		int type;
		int buffer_type;
		VertexDescriptor desc;
		
		void alloc( unsigned int bytes_per_vertex, IndexType max_vertices, IndexType max_indices = 0 );
		void reset();
		void dealloc();
		unsigned int bytes_used();
		VertexType * request( IndexType num_vertices, int dont_advance_pointer = 0 );
		void append_indices( IndexType * inIndices, IndexType num_indices );
		
		void create( unsigned int vertex_stride, IndexType max_vertices, IndexType max_indices, renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type = renderer::BUFFER_STATIC );
		void destroy();
		void update();
		void draw_elements();
		void draw();
	}; // VertexStream

}; // namespace renderer

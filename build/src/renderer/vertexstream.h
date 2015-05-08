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

namespace gemini
{
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

	} // namespace renderer
} // namespace gemini
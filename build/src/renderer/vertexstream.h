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

		LIBRARY_EXPORT VertexStream();
		LIBRARY_EXPORT ~VertexStream();
		LIBRARY_EXPORT void alloc( IndexType max_vertices, IndexType max_indices = 0 );
		LIBRARY_EXPORT void reset();
		LIBRARY_EXPORT void dealloc();
		LIBRARY_EXPORT unsigned int bytes_used();
		LIBRARY_EXPORT VertexType * request( IndexType num_vertices, int dont_advance_pointer = 0 );
		LIBRARY_EXPORT VertexType * operator[](int index);
		LIBRARY_EXPORT void append_indices( IndexType * inIndices, IndexType num_indices );
		LIBRARY_EXPORT bool has_room( unsigned int num_vertices, unsigned int num_indices ) const;

		LIBRARY_EXPORT void create( IndexType max_vertices, IndexType max_indices, renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type = renderer::BUFFER_STATIC );
		LIBRARY_EXPORT void destroy();
		LIBRARY_EXPORT void update();
		LIBRARY_EXPORT void draw_elements();
		LIBRARY_EXPORT void draw();
		LIBRARY_EXPORT void fill_data( VertexType * vertex_source, unsigned int vertex_count, IndexType * index_source, unsigned int index_count );

	}; // VertexStream

} // namespace renderer

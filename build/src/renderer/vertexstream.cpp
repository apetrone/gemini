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
#include "vertexstream.h"
#include "renderer.h"

#include <core/typedefs.h>
#include <core/logging.h>

#include <string.h> // for memset
#include <stdio.h> // for printf

namespace renderer
{
	VertexStream::VertexStream()
	{
		total_vertices = 0;
		total_indices = 0;
		vertices = 0;
		last_vertex = 0;
		last_index = 0;
		highest_index = 0;
		indices = 0;
		vertexbuffer = 0;
	} // VertexStream
	
	VertexStream::~VertexStream()
	{
		destroy();
	} // ~VertexStream

	void VertexStream::alloc( IndexType max_vertices, IndexType max_indices )
	{
		total_vertices = max_vertices;
		total_indices = 0;
		vertices = (VertexType*)ALLOC(vertex_stride*total_vertices);
		last_vertex = 0;
		last_index = 0;
		highest_index = 0;
		indices = 0;

		if ( max_indices > 0 )
		{
			indices = (IndexType*)ALLOC( sizeof(IndexType) * max_indices );
			total_indices = max_indices;
		}
	} // alloc

	void VertexStream::reset()
	{
		last_vertex = 0;
		last_index = 0;
		highest_index = 0;
		
		// I'm not sure we WANT to reset the vertexbuffer each time
		// we reset the stream -- in this fashion.
		if (vertexbuffer)
		{
			vertexbuffer->index_count = 0;
			vertexbuffer->vertex_count = 0;
		}
	} // reset

	void VertexStream::dealloc()
	{
		if ( vertices )
		{
			DEALLOC(vertices);
			vertices = 0;
		}

		if ( indices )
		{
			DEALLOC(indices);
			indices = 0;
		}
	} // dealloc

	unsigned int VertexStream::bytes_used()
	{
		return (total_vertices * vertex_stride) + (total_indices * sizeof(IndexType));
	} // bytes_used

	renderer::VertexType * VertexStream::request( IndexType num_vertices, int dont_advance_pointer )
	{
		if ( _debug_flags > 0 )
		{
			LOGE( "Requesting (%i) vertices from VertexStream; must call create() first!\n", num_vertices );
		}

		VertexType * vptr;

		if ( num_vertices > total_vertices )
		{
			// this is not possible with the total: cannot force more vertices
			return 0;
		}

		if ( last_vertex + num_vertices > total_vertices )
		{
			// we cannot accomodate that request
			return 0;
		}

		vptr = &vertices[ (vertex_stride * last_vertex) ];

		if ( !dont_advance_pointer )
		{
			last_vertex += num_vertices;
		}

		return vptr;
	} // request
	
	VertexType * VertexStream::operator[](int index)
	{
		VertexType * vptr = 0;
		
		// requested a vertex that hasn't been accessed
		if ( (unsigned int)index > last_vertex )
		{
			return 0;
		}
		
		vptr = &vertices[ (vertex_stride * index) ];
		
		return vptr;
	} // operator []

	void VertexStream::append_indices( IndexType * inIndices, IndexType num_indices )
	{
		IndexType numStartingIndices;
		IndexType j;
#if 0
		if ( (highest_index + num_indices) > total_indices )
		{
			reset();
			return;
		}
#endif
		numStartingIndices = highest_index;

		// copy index data
		for( j = 0; j < num_indices; ++j, ++last_index )
		{
			if ( numStartingIndices + inIndices[j] > highest_index )
			{
				highest_index = (numStartingIndices + inIndices[j]);
			}
#if 0
			if ( last_index >= total_indices )
			{
				break;
			}
#endif
			indices[ last_index ] = (numStartingIndices + inIndices[ j ]);
		}

		highest_index++;
	} // append_indices
	
	
	// determine if this vertexstream has room
	bool VertexStream::has_room( unsigned int num_vertices, unsigned int num_indices ) const
	{
		if ( (num_vertices + last_vertex) >= total_vertices )
		{
			return false;
		}

		if ( (total_indices > 0) && ((num_indices + last_index) >= total_indices) )
		{
			return false;
		}
		
		return true;
	} // has_room

	void VertexStream::create( IndexType max_vertices, IndexType max_indices, renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type )
	{
		vertex_stride = desc.calculate_vertex_stride();
		reset();
		
		if ( desc.attribs == 0 )
		{
			LOGE( "VertexStream description NOT SET!\n" );
		}

		//printf( "template_vertex_size = %i bytes <-> vertexStride = %i bytes\n", sizeof(VertexType), vertexStride );

		alloc( max_vertices, max_indices );
		
		this->vertexbuffer = renderer::driver()->vertexbuffer_create(
			this->desc,
			draw_type,
			buffer_type,
			vertex_stride,
			max_vertices,
			max_indices );

		_debug_flags = 0;
	} // create

	void VertexStream::destroy()
	{
		this->dealloc();
		if ( renderer::driver() && this->vertexbuffer )
		{
			renderer::driver()->vertexbuffer_destroy( this->vertexbuffer );
			this->vertexbuffer = 0;
		}
		
	} // destroy

	void VertexStream::update()
	{
		if ( last_vertex >= total_vertices )
		{
			last_vertex = total_vertices-1;
		}
		
		if ( (total_indices > 0) && (last_index >= total_indices) )
		{
			last_index = total_indices-1;
		}
		
		if ( last_vertex > 0 )
		{
			renderer::driver()->vertexbuffer_upload_data( this->vertexbuffer, this->vertex_stride, this->last_vertex, this->vertices, this->last_index, this->indices );
		}
	} // update

	void VertexStream::draw_elements()
	{
		renderer::driver()->vertexbuffer_draw_indices( this->vertexbuffer, this->last_index );
	} // draw_elements

	void VertexStream::draw()
	{
		renderer::driver()->vertexbuffer_draw( this->vertexbuffer, this->last_vertex );
	} // draw
	
	void VertexStream::fill_data( VertexType * vertex_source, unsigned int vertex_count, IndexType * index_source, unsigned int index_count )
	{
		// if you hit this assert, more vertices were copied to the buffer than
		// it holds and memory is corrupt now. nicely done.
		assert( vertex_count + this->last_vertex < total_vertices );
		
		memcpy( this->vertices, vertex_source, this->vertex_stride * vertex_count );
		this->last_vertex = vertex_count;
				
		if ( index_count > 0 )
		{
			memcpy( this->indices, index_source, sizeof(IndexType)*index_count );
			this->last_index = index_count;
		}
	} // fill-data
} // namespace renderer
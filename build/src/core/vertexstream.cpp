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
#include "typedefs.h"
#include "vertexstream.hpp"
#include "renderer.hpp"

#include <string.h> // for memset
#include <stdio.h> // for printf
#include "log.h"

namespace renderer
{
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
	}

	void VertexStream::reset()
	{
		last_vertex = 0;
		last_index = 0;
		highest_index = 0;
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
		if ( index > last_vertex )
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
		if ( num_vertices + last_vertex >= total_vertices )
		{
			return false;
		}

		if ( (total_indices > 0) && (num_indices + last_index >= total_indices) )
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
		renderer::driver()->vertexbuffer_destroy( this->vertexbuffer );
		this->vertexbuffer = 0;
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
		
		renderer::driver()->vertexbuffer_bufferdata( this->vertexbuffer, this->vertex_stride, this->last_vertex, this->vertices, this->last_index, this->indices );
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
		memcpy( this->vertices, vertex_source, this->vertex_stride * vertex_count );
		this->last_vertex = vertex_count;
				
		if ( index_count > 0 )
		{
			memcpy( this->indices, index_source, sizeof(IndexType)*index_count );
			this->last_index = index_count;
		}
	} // fill-data
}; // namespace renderer

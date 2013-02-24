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
	void VertexStream::alloc( unsigned int bytes_per_vertex, IndexType max_vertices, IndexType max_indices )
	{
		total_vertices = max_vertices;
		vertex_stride = bytes_per_vertex;
		total_indices = 0;
		vertices = new VertexType[ (vertex_stride*total_vertices) ];
		last_vertex = 0;
		last_index = 0;
		highest_index = 0;
		indices = 0;

		if ( max_indices > 0 )
		{
			indices = new IndexType[ max_indices ];
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
			delete [] vertices;
			vertices = 0;
		}

		if ( indices )
		{
			delete [] indices;
			indices = 0;
		}
	} // dealloc

	unsigned int VertexStream::bytes_used()
	{
		return total_vertices * vertex_stride;
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

	void VertexStream::append_indices( IndexType * inIndices, IndexType num_indices )
	{
		IndexType numStartingIndices;
		IndexType j;

		if ( (highest_index + num_indices) > total_indices )
		{
			reset();
			return;
		}

		numStartingIndices = highest_index;

		// copy index data
		for( j = 0; j < num_indices; ++j, ++last_index )
		{
			if ( numStartingIndices + inIndices[j] > highest_index )
			{
				highest_index = (numStartingIndices + inIndices[j]);
			}

			if ( last_index >= total_indices )
			{
				break;
			}
			
			indices[ last_index ] = (numStartingIndices + inIndices[ j ]);
		}

		highest_index++;
	} // append_indices

	void VertexStream::create( unsigned int vertex_stride, IndexType max_vertices, IndexType max_indices, renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type )
	{	
		if ( desc.attribs == 0 )
		{
			LOGE( "VertexStream description NOT SET!\n" );
		}

		//printf( "template_vertex_size = %i bytes <-> vertexStride = %i bytes\n", sizeof(VertexType), vertexStride );

		alloc( vertex_stride, max_vertices, max_indices );
		
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
		
		if ( last_index >= total_indices )
		{
			last_index = total_indices-1;
		}
		
		renderer::driver()->vertexbuffer_bufferdata( this->vertexbuffer, vertex_stride, this->last_vertex, this->vertices, this->last_index, this->indices );
	} // update

	void VertexStream::draw_elements()
	{
		renderer::driver()->vertexbuffer_draw_indices( this->vertexbuffer, this->last_index );
	} // draw_elements

	void VertexStream::draw()
	{
#if 0
		gl.BindVertexArray( vao );

		gl.DrawArrays( type, 0, this->lastVertex );
		gl.CheckError( "DrawArrays" );
		gl.BindVertexArray( 0 );
#endif
	} // draw

	// VertexTypeDescriptor
	
	
	unsigned int VertexDescriptor::size[ VD_TOTAL ] =
	{
		sizeof(float) * 2,
		sizeof(float) * 3,
		sizeof(float) * 4,
		sizeof(unsigned char) * 3,
		sizeof(unsigned char) * 4,
		sizeof(unsigned int)
	};
	
	unsigned int VertexDescriptor::elements[ VD_TOTAL ] =
	{
		2,
		3,
		4,
		3,
		4,
		1
	};
	
	VertexDescriptor::VertexDescriptor()
	{
		id = 0;
		memset( description, 0, sizeof(VertexDescriptorType) * MAX_DESCRIPTORS );
	}

	void VertexDescriptor::add( VertexDescriptorType desc )
	{
		description[ id ] = desc;
		++id;

		if ( id >= MAX_DESCRIPTORS-1 )
		{
			printf( "Reached MAX_DESCRIPTORS. Resetting\n" );
			id = 0;
		}

		attribs = id;
	}

	VertexDescriptorType VertexDescriptor::get( int i )
	{
		return description[ i ];
	}

	void VertexDescriptor::reset()
	{
		attribs = id;
		id = 0;
	}
}; // namespace renderer

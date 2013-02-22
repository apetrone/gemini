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
		totalVertices = max_vertices;
		vertexStride = bytes_per_vertex;
		totalIndices = 0;
		vertices = new VertexType[ (vertexStride*totalVertices) ];
		lastVertex = 0;
		lastIndex = 0;
		highestIndex = 0;
		indices = 0;

		if ( max_indices > 0 )
		{
			indices = new IndexType[ max_indices ];
			totalIndices = max_indices;
		}
	}

	void VertexStream::reset()
	{
		lastVertex = 0;
		lastIndex = 0;
		highestIndex = 0;
	}

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
	}

	unsigned int VertexStream::bytes_used()
	{
		return totalVertices * vertexStride;
	}

	VertexStream::VertexType * VertexStream::request( IndexType num_vertices, int dont_advance_pointer )
	{
		if ( _debug_flags > 0 )
		{
			LOGE( "Requesting (%i) vertices from VertexStream; must call create() first!\n", num_vertices );
		}

		VertexType * vptr;

		if ( num_vertices > totalVertices )
		{
			//fprintf( stdout, "Cannot have more vertices!\n" );
			// this is not possible with the total
			return 0;
		}

		if ( lastVertex + num_vertices > totalVertices )
			// we cannot accomodate that request
			return 0;

		vptr = &vertices[ (vertexStride * lastVertex) ];

		if ( !dont_advance_pointer )
			lastVertex += num_vertices;

		return vptr;
	}

	void VertexStream::append_indices( IndexType * inIndices, IndexType num_indices )
	{
		IndexType numStartingIndices;
		IndexType j;

		if ( (highestIndex + num_indices) > totalIndices )
		{
			reset();
			return;
		}

		numStartingIndices = highestIndex;

		// copy index data
		for( j = 0; j < num_indices; ++j, ++lastIndex )
		{
			if ( numStartingIndices + inIndices[j] > highestIndex )
				highestIndex = (numStartingIndices + inIndices[j]);

			if ( lastIndex >= totalIndices )
			{
				break;
			}
			
			indices[ lastIndex ] = (numStartingIndices + inIndices[ j ]);
		}

		highestIndex++;
	} // appendIndices

	void VertexStream::create( unsigned int vertex_stride, IndexType max_vertices, IndexType max_indices, renderer::VertexBufferDrawType draw_type, renderer::VertexBufferBufferType buffer_type )
	{	
		if ( desc.attribs == 0 )
		{
			LOGE( "VertexStream description NOT SET!\n" );
		}

		//printf( "template_vertex_size = %i bytes <-> vertexStride = %i bytes\n", sizeof(VertexType), vertexStride );

		alloc( vertexStride, max_vertices, max_indices );
		
		this->vertexbuffer = renderer::driver()->vertexbuffer_create(
			this->desc,
			draw_type,
			buffer_type,
			vertex_stride,
			max_vertices,
			max_indices );

		_debug_flags = 0;
	}

	void VertexStream::destroy()
	{
		this->dealloc();
		renderer::driver()->vertexbuffer_destroy( this->vertexbuffer );
	}

	void VertexStream::update()
	{
		if ( lastVertex >= totalVertices )
		{
			lastVertex = totalVertices-1;
		}
		
		if ( lastIndex >= totalIndices )
		{
			lastIndex = totalIndices-1;
		}
		
//		renderer::driver()->vertexbuffer_bufferdata( vertexStride * this->lastVertex, this->vertices, sizeof(IndexType) * this->lastIndex, this->indices );
#if 0
		gl.BindVertexArray( vao );

		gl.BindBuffer( GL_ARRAY_BUFFER, vbo[0] );
		gl.CheckError( "BindBuffer GL_ARRAY_BUFFER" );
		gl.BufferData( GL_ARRAY_BUFFER, vertexStride * this->lastVertex, this->vertices, buffer_type );

		if ( vbo[1] != 0 )
		{
			gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo[1] );
			gl.CheckError( "BindBuffer GL_ELEMENT_ARRAY_BUFFER" );
			gl.BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * this->lastIndex, this->indices, buffer_type );
		}

		gl.BindVertexArray( 0 );
		gl.BindBuffer( GL_ARRAY_BUFFER, 0 );
		gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
#endif
	}

	void VertexStream::draw_elements()
	{
#if 0
		gl.BindVertexArray( vao );
		gl.CheckError( "BindVertexArray" );
		gl.DrawElements( type, this->lastIndex, GL_UNSIGNED_INT, 0 );
		gl.CheckError( "DrawElements" );
		gl.BindVertexArray( 0 );
#endif
	}

	void VertexStream::draw()
	{
#if 0
		gl.BindVertexArray( vao );

		gl.DrawArrays( type, 0, this->lastVertex );
		gl.CheckError( "DrawArrays" );
		gl.BindVertexArray( 0 );
#endif
	}

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

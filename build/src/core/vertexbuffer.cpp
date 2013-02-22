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
#include <vertexbuffer.hpp>
#include "renderer.hpp"

#include <string.h> // for memset
#include <stdio.h> // for printf
#include "log.h"

namespace renderer
{
	void VertexBuffer::alloc( unsigned int bytes_per_vertex, IndexType max_vertices, IndexType max_indices )
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

	void VertexBuffer::reset()
	{
		lastVertex = 0;
		lastIndex = 0;
		highestIndex = 0;
	}

	void VertexBuffer::dealloc()
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

	unsigned int VertexBuffer::bytes_used()
	{
		return totalVertices * vertexStride;
	}

	VertexBuffer::VertexType * VertexBuffer::request( IndexType num_vertices, int dont_advance_pointer )
	{
		if ( _debug_flags > 0 )
		{
			LOGE( "Requesting (%i) vertices from VertexBuffer; must call create() first!\n", num_vertices );
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

	void VertexBuffer::append_indices( IndexType * inIndices, IndexType num_indices )
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

	void VertexBuffer::create( unsigned int vertexStride, IndexType max_vertices, IndexType max_indices, int drawtype, int buffertype )
	{
		unsigned int attribID = 0;
		unsigned int attribSize = 0;
		unsigned int num_elements = 0;
		unsigned int normalized = 0;
		unsigned int offset = 0;
#if 0
		GLenum attrib_type;
#endif
		VertexDescriptor descriptor;
		
		if ( desc.attribs == 0 )
		{
			LOGE( "VertexBuffer description NOT SET!\n" );
		}

		if ( buffertype == 0 )
		{
#if 0
			buffertype = GL_STATIC_DRAW;
#endif
		}
		
		//printf( "template_vertex_size = %i bytes <-> vertexStride = %i bytes\n", sizeof(VertexType), vertexStride );

		alloc( vertexStride, max_vertices, max_indices );
		type = drawtype;
		buffer_type = buffertype;

		vbo[0] = 0;
		vbo[1] = 0;
		
//		this->geometry_stream = renderer::driver()->geometrystream_create( desc, renderer::STATIC_DRAW, vertexStride * max_vertices, sizeof(IndexType) * max_indices );

#if 0
		gl.GenVertexArrays( 1, &vao );
		gl.BindVertexArray( vao );

		gl.GenBuffers( 1, vbo );
		gl.BindBuffer( GL_ARRAY_BUFFER, vbo[0] );
		gl.CheckError( "BindBuffer" );

		gl.BufferData( GL_ARRAY_BUFFER, vertexStride * max_vertices, 0, buffer_type );
		gl.CheckError( "BufferData" );
#endif
		if ( max_indices > 0 )
		{
#if 0
			gl.GenBuffers( 1, &vbo[1] );
			gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo[1] );
			gl.CheckError( "BindBuffer" );

			gl.BufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * max_indices, 0, buffer_type );
			gl.CheckError( "BufferData" );
#endif
		
		}

		desc.reset();
#if 0
		attrib_type = GL_INVALID_ENUM;

		for( unsigned int i = 0; i < desc.attribs; ++i )
		{
			descriptor = desc.description[i];
			if ( descriptor == VD_FLOAT2 )
			{
				attrib_type = GL_FLOAT;
				normalized = GL_FALSE;
			}
			else if ( descriptor == VD_FLOAT3 )
			{
				attrib_type = GL_FLOAT;
				normalized = GL_FALSE;
			}
			else if ( descriptor == VD_UNSIGNED_INT )
			{
				attrib_type = GL_UNSIGNED_INT;
				normalized = GL_FALSE;
			}
			else if ( descriptor == VD_UNSIGNED_BYTE3 )
			{
				attrib_type = GL_UNSIGNED_BYTE;
				normalized = GL_TRUE;
			}
			else if ( descriptor == VD_UNSIGNED_BYTE4 )
			{
				attrib_type = GL_UNSIGNED_BYTE;
				normalized = GL_TRUE;
			}

			num_elements = VertexTypeDescriptor::elements[ descriptor ];
			attribSize = VertexTypeDescriptor::size[ descriptor ];
			gl.VertexAttribPointer( attribID, num_elements, attrib_type, normalized, vertexStride, (void*)offset );
			gl.CheckError( "VertexAttribPointer" );

			gl.EnableVertexAttribArray( attribID );
			gl.CheckError( "EnableVertexAttribArray" );

			
			offset += attribSize;
			++attribID;
		}

		gl.BindBuffer( GL_ARRAY_BUFFER, 0 );
		gl.BindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		gl.BindVertexArray( 0 );
#endif
		
		_debug_flags = 0;
	}

	void VertexBuffer::destroy()
	{
		this->dealloc();
//		renderer::driver()->geometrystream_destroy( this->geometry_stream );
#if 0
		if ( vao != 0 )
		{
			gl.DeleteVertexArrays( 1, &vao );
		}
		
		if ( vbo[0] != 0 )
		{
			gl.DeleteBuffers( 1, vbo );
		}

		if ( vbo[1] != 0 )
		{
			gl.DeleteBuffers( 1, &vbo[1] );
		}
#endif
	}

	void VertexBuffer::update()
	{
		if ( lastVertex >= totalVertices )
		{
			lastVertex = totalVertices-1;
		}
		
		if ( lastIndex >= totalIndices )
		{
			lastIndex = totalIndices-1;
		}
		
//		renderer::driver()->geometrystream_bufferdata( vertexStride * this->lastVertex, this->vertices, sizeof(IndexType) * this->lastIndex, this->indices );
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

	void VertexBuffer::draw_elements()
	{
#if 0
		gl.BindVertexArray( vao );
		gl.CheckError( "BindVertexArray" );
		gl.DrawElements( type, this->lastIndex, GL_UNSIGNED_INT, 0 );
		gl.CheckError( "DrawElements" );
		gl.BindVertexArray( 0 );
#endif
	}

	void VertexBuffer::draw()
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

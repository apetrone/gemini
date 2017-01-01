// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "vertexdescriptor.h"

#include <core/typedefs.h>

namespace render2
{
	VertexDescriptor::VertexDescriptor()
	{
		id = 0;
		reset();
		memset(description, 0, sizeof(VertexDataType) * MAX_VERTEX_DESCRIPTORS);
	}

	VertexDescriptor::VertexDescriptor(const VertexDescriptor& other)
	{
		*this = other;
	}

	void VertexDescriptor::add(const char* name, const VertexDataType& type, size_t element_count)
	{
		InputDescription descriptor;
		descriptor.name = name;
		descriptor.type = type;
		descriptor.element_count = element_count;

		description[ id++ ] = descriptor;

		if ( id >= MAX_VERTEX_DESCRIPTORS-1 )
		{
			printf( "Reached MAX_DESCRIPTORS. Resetting\n" );
			id = 0;
		}

		total_attributes = id;
	} // add

	const VertexDescriptor::InputDescription& VertexDescriptor::operator[](size_t index) const
	{
		return description[ index ];
	} // operator[]

	void VertexDescriptor::reset()
	{
		if ( id > 0 )
		{
			total_attributes = id;
		}
		id = 0;
	} // reset

	size_t VertexDescriptor::size() const
	{
		return total_attributes;
	} // size

	const VertexDescriptor& VertexDescriptor::operator= (const VertexDescriptor & other)
	{
		total_attributes = other.total_attributes;
		id = other.id;

		for( unsigned int index = 0; index < MAX_VERTEX_DESCRIPTORS; ++index )
		{
			description[index] = other.description[index];
		}

		return *this;
	} // operator=
} // namespace render2

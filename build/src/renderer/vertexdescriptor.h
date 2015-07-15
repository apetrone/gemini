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
#pragma once

#include <core/stackstring.h>
#include <core/typedefs.h>

namespace render2
{
	const size_t MAX_VERTEX_DESCRIPTORS = 8;
	
	enum VertexDataType
	{
		VD_FLOAT = 0,
		VD_INT,
		VD_UNSIGNED_INT,
		VD_UNSIGNED_BYTE,
				
		VD_TOTAL
	};
	
	// describes the layout of the vertex stream
	struct VertexDescriptor
	{
		struct InputDescription
		{
			core::StackString<32> name;
			VertexDataType type;
			size_t element_count;
		};
		
		unsigned char id;
		unsigned char total_attributes;
		InputDescription description[ MAX_VERTEX_DESCRIPTORS ];
		
		VertexDescriptor();
		VertexDescriptor(const VertexDescriptor& other);
		void add(const char* name, const VertexDataType& type, size_t element_count);
		const InputDescription& operator[](int index) const;
		void reset();
		size_t size() const;
		
		const VertexDescriptor& operator= (const VertexDescriptor& other);
	}; // VertexDescriptor
} // namespace render2

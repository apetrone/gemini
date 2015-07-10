// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

namespace renderer
{
	struct VertexBuffer
	{
		int vertex_count;
		int index_count;
		
		VertexBuffer()
		{
			vertex_count = 0;
			index_count = 0;
		}
	}; // VertexBuffer
} // namespace renderer

namespace render2
{
	// 1. uploading all buffer data to GPU in one call
	// 2. uploading a part of the buffer data to the GPU in one call
	// 3. retrieving all or part of the buffer data via function to populate it
	class Buffer
	{
		enum
		{
			BUFFER_IS_DIRTY = 1
		};
		
	public:
		
		Buffer() :
		max_size(0),
		flags(0)
		{
		}
		
		virtual ~Buffer() {}
		void clear_flag(uint32_t flag);
		
	public:
		
		size_t max_size_bytes() const { return max_size; }
		bool is_dirty() const { return flags & BUFFER_IS_DIRTY; }
		
	protected:
		uint32_t max_size;
		uint32_t flags;
	};
} // namespace render2
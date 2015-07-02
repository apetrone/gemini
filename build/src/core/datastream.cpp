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
#include "datastream.h"

#ifndef memcpy
	#include <string.h>
#endif

namespace core
{
	namespace util
	{
		void MemoryStream::init(void* buffer, size_t buffer_length)
		{
			data = (uint8_t*)buffer;
			data_length = buffer_length;
			offset = 0;
		}
		
		void MemoryStream::rewind()
		{
			offset = 0;
		}
		
		void MemoryStream::clear()
		{
			memset(data, 0, data_length);
		}
		
		size_t MemoryStream::current_offset() const { return offset; }
		
		uint8_t* MemoryStream::get_data() const { return data; }
		size_t MemoryStream::get_data_size() const { return data_length; }
		
		size_t MemoryStream::read(void* destination, size_t num_bytes)
		{
			if ( !data )
			{
				return 0;
			}
			
			// check for read violation
			assert((offset + num_bytes) <= data_length);
			
			// copy memory and advance the pointer
			memcpy(destination, &data[offset], num_bytes);
			offset += num_bytes;
			
			return num_bytes;
		}
		
		size_t MemoryStream::write(const void* source, size_t num_bytes)
		{
			if ( !data )
			{
				return 0;
			}
			
			// check for write violation
			assert((offset + num_bytes) <= data_length);
			
			// copy memory and advance pointer
			memcpy(&((char*)data)[offset], source, num_bytes);
			offset += num_bytes;
			
			return num_bytes;
		}
		
		void MemoryStream::seek(size_t requested_offset, bool is_absolute)
		{
			if (data)
			{
				if (is_absolute)
				{
					offset = requested_offset;
				}
				else
				{
					offset += requested_offset;
				}
			}
		}
		
		
		

		uint8_t* ResizableMemoryStream::get_data() const
		{
			return (uint8_t*)data.data();
		}
		
		size_t ResizableMemoryStream::get_data_size() const
		{
			return data.size();
		}
		
		size_t ResizableMemoryStream::read(void* destination, size_t length)
		{
			return 0;
		}
		
		size_t ResizableMemoryStream::write(const void* source, size_t length)
		{
			data.resize(data.size() + length);
			memcpy(data.data()+offset, source, length);
			
			offset += length;

			return length;
		}
		
		void ResizableMemoryStream::seek(size_t requested_offset, bool is_absolute)
		{
			if (is_absolute)
			{
				offset = requested_offset;
			}
			else
			{
				offset += requested_offset;
			}
		}
	} // namespace util
} // namespace core
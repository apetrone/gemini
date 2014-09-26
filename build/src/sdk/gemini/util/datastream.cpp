// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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

#include "datastream.h"

namespace util
{
	void MemoryStream::init(char* buffer, size_t buffer_length)
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
	
}
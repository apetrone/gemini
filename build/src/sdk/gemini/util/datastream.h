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
#pragma once

#include <vector>

#include <gemini/typedefs.h>

#include "datastream.h"

namespace util
{
	class DataStream
	{
	public:
		virtual ~DataStream() {}
		
		// fetch data pointer
		virtual uint8_t* get_data() const = 0;
		
		// get data length; 0 if not known
		virtual size_t get_data_size() const = 0;
		
		// read a number of bytes into the destination buffer
		// returns the number of bytes read
		virtual size_t read(void* destination, size_t length) = 0;
		
		// write data to the stream
		// returns the number of bytes written
		virtual size_t write(const void* data, size_t length) = 0;
		
		// seek to an offset location within the stream
		// if is_absolute is false, the seek happens relative to the current position
		virtual void seek(size_t offset, bool is_absolute) = 0;
		
		// flush the data stream
		virtual void flush() = 0;
	};
	
	class MemoryStream : public DataStream
	{
	public:
		uint8_t* data;
		size_t data_length;
		size_t offset;
		
		MemoryStream() : data(0), data_length(0), offset(0) {}
		virtual ~MemoryStream() {}
		
		void init(char* buffer, size_t buffer_length);
		void rewind();
		void clear();
		size_t current_offset() const;
		template <class Type>
		
		int write( const Type & value )
		{
			return write( &value, sizeof(Type) );
		}
		
		
		template <class Type>
		int read( Type & destination )
		{
			return read( &destination, sizeof(Type) );
		}
		
		virtual uint8_t* get_data() const;
		virtual size_t get_data_size() const;
		virtual size_t read(void* destination, size_t length);
		virtual size_t write(const void* data, size_t length);
		virtual void seek(size_t offset, bool is_absolute);
		virtual void flush() {}
	};
	
	class ResizableMemoryStream : public MemoryStream
	{
		std::vector<char> data;
		size_t offset;
		
	public:
		ResizableMemoryStream() : offset(0) {}
		
		virtual uint8_t* get_data() const;
		virtual size_t get_data_size() const;
		virtual size_t read(void* destination, size_t length);
		virtual size_t write(const void* data, size_t length);
		virtual void seek(size_t offset, bool is_absolute);
		virtual void flush() {}
	};
}
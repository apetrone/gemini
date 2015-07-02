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

#include "datastream.h"
#include "typedefs.h"

#include <vector>

namespace core
{
	namespace util
	{
		class LIBRARY_EXPORT DataStream
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
		
		class LIBRARY_EXPORT MemoryStream : public DataStream
		{
		public:
			uint8_t* data;
			size_t data_length;
			size_t offset;
			
			MemoryStream() : data(0), data_length(0), offset(0) {}
			virtual ~MemoryStream() {}
			
			void init(void* buffer, size_t buffer_length);
			void rewind();
			void clear();
			size_t current_offset() const;
			template <class Type>
			
			int write(const Type & value)
			{
				return write(&value, sizeof(Type));
			}
			
			
			template <class Type>
			int read(Type & destination)
			{
				return read(&destination, sizeof(Type));
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
			LIBRARY_EXPORT ResizableMemoryStream() : offset(0) {}
			
			LIBRARY_EXPORT virtual uint8_t* get_data() const;
			LIBRARY_EXPORT virtual size_t get_data_size() const;
			LIBRARY_EXPORT virtual size_t read(void* destination, size_t length);
			LIBRARY_EXPORT virtual size_t write(const void* data, size_t length);
			LIBRARY_EXPORT virtual void seek(size_t offset, bool is_absolute);
			LIBRARY_EXPORT virtual void flush() {}
		};
	} // namespace util
} // namespace core
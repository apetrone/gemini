// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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


#include <core/array.h>
#include <core/typespec.h>

namespace gemini
{
	struct FieldCollector : public ArchiveInterface<FieldCollector>
	{
		gemini::Allocator allocator;
		Array<gemini::string> fields;
		Array<intptr_t> offsets;
		Array<TypeSpecInfo*> info;

		FieldCollector()
			: allocator(gemini::memory_allocator_default(MEMORY_ZONE_DEFAULT))
			, fields(allocator)
			, offsets(allocator)
			, info(allocator)
		{
			IsSaving = 1;
		}

		template <class T>
		void save_pair(FieldKeyValuePair<T>& pair)
		{
			TypeSpecInfo* typeinfo = typespec_make_info<T>();
			info.push_back(typeinfo);
			fields.push_back(string_create(allocator, pair.name));
			offsets.push_back(pair.offset);
		}

		template <class T>
		void save(T& value)
		{
		}

		template <class T>
		void load(T& value)
		{
		}

		~FieldCollector()
		{
			for (size_t index = 0; index < fields.size(); ++index)
			{
				string_destroy(allocator, fields[index]);
			}
		}
	}; // FieldCollector
} // namespace gemini

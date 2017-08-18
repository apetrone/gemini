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


#include <core/hashset.h>
#include <core/logging.h>
#include <core/mathlib.h>

namespace gemini
{
	class KeyValueArchive : public ArchiveInterface<KeyValueArchive>
	{
	public:
		KeyValueArchive(gemini::Allocator& _allocator)
			: allocator(_allocator)
			, items(_allocator)
		{
			IsLoading = 1;
		}

		void set_item(gemini::string key, gemini::string value)
		{
			items[key] = value;
		}


		template <class T>
		void load_pair(const FieldKeyValuePair<T>& pair)
		{
			instance().prologue(pair);
			instance().load(const_cast<T&>(pair.value));
			instance().epilogue(pair);
		}

		template <class T>
		void prologue(const FieldKeyValuePair<T>& pair)
		{
			set_next_name(pair.name);
		}

		template <class T>
		void epilogue(const FieldKeyValuePair<T>& /*pair*/)
		{
			string_destroy(allocator, current_field_name);
		}


		template <class T>
		void load(T& value)
		{
			// If you hit this, then the archive couldn't resolve the correct type.
			assert(0);
		}

	private:
		void set_next_name(const char* name)
		{
			current_field_name = string_create(allocator, name);
		}

		gemini::Allocator& allocator;
		gemini::string current_field_name;
		HashSet<gemini::string, gemini::string> items;

		KeyValueArchive& operator=(const KeyValueArchive& other) = delete;
	}; // KeyValueArchive


	template <>
	void KeyValueArchive::load(uint32_t& value)
	{
		if (items.has_key(current_field_name))
		{
			gemini::string field_value = items.get(current_field_name);
			value = static_cast<uint32_t>(atoi(field_value.c_str()));
		}
	}

	template <>
	void KeyValueArchive::load(float& value)
	{
		if (items.has_key(current_field_name))
		{
			gemini::string field_value = items.get(current_field_name);
			value = static_cast<float>(atof(field_value.c_str()));
		}
	}

	template <>
	void KeyValueArchive::load(char*& value)
	{
		if (items.has_key(current_field_name))
		{
			gemini::string field_value = items.get(current_field_name);
			value = new char[field_value.size() + 1];
			value[field_value.size()] = '\0';
			core::str::copy(value, field_value.string_data, field_value.string_data_size);
		}
	}

	template <>
	void KeyValueArchive::load(glm::vec4& value)
	{
		if (items.has_key(current_field_name))
		{
			gemini::string field_value = items.get(current_field_name);
			int results = sscanf(field_value.c_str(),
				"%f %f %f %f",
				&value.x, &value.y, &value.z, &value.w);
			if (results < 4)
			{
				LOGV("Error reading vec4.\n");
			}
		}
	}

} // namespace gemini

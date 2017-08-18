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


namespace gemini
{
	template <class ArchiveType>
	struct ArchiveInterface
	{
		ArchiveInterface()
		{
		}

		ArchiveType& instance()
		{
			return *static_cast<ArchiveType*>(this);
		}

		template <class T>
		ArchiveType& operator& (const FieldKeyValuePair<T>& pair)
		{
			if (IsSaving)
			{
				instance().save_pair(pair);
			}
			else if (IsLoading)
			{
				instance().load_pair(pair);
			}
			return instance();
		}

		template <class T>
		ArchiveType& operator<< (const T& value)
		{
			FieldKeyValuePair<T> pair("Unnamed", std::forward<T>(const_cast<T&>(value)), 0);
			SerializeDispatcher<ArchiveType, T>(instance(), pair);
			return instance();
		}

		template <class T>
		void save_pair(const FieldKeyValuePair<T>& pair)
		{
			instance().save(pair.value);
		}

		template <class T>
		ArchiveType& operator>> (T& value)
		{
			FieldKeyValuePair<T> pair("Unnamed", std::forward<T>(value), 0);
			SerializeDispatcher(instance(), pair);
			return instance();
		}

		template <class T>
		void load_pair(FieldKeyValuePair<T>& pair)
		{
			instance().load(const_cast<T&>(pair.value));
		}

		template <class T>
		void save(const T& /*value*/)
		{
			//' If you hit this, no save function was specified for the derived
			// class being used.
			assert(0);
		}

		template <class T>
		void load(T& /*value*/)
		{
			//' If you hit this, no load function was specified for the derived
			// class being used.
			assert(0);
		}

		uint16_t IsLoading = 0;
		uint16_t IsSaving = 0;
	}; // ArchiveInterface
} // namespace gemini

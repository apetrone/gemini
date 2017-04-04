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

#include <core/mem.h>
#include <core/str.h>

#include <core/mathlib.h>

namespace gemini
{
	struct DebugVarBase
	{
		const char* name;
		struct DebugVarBase* next;

		DebugVarBase(const char* variable_name);
		virtual const char* to_string() = 0;
	};


	template <class T>
	struct DebugVar : public DebugVarBase
	{
		T* data;

		DebugVar(const char* name, T* value)
			: DebugVarBase(name)
			, data(value)
		{

		}

		virtual const char* to_string()
		{
			assert(0);
		}
	};

	template <>
	const char* DebugVar<float>::to_string()
	{
		return core::str::format("%s: %2.2f", name, *data);
	}

	template <>
	const char* DebugVar<glm::vec2>::to_string()
	{
		return core::str::format("%s: [%2.2f, %2.2f]", name, (*data).x, (*data).y);
	}

	template <>
	const char* DebugVar<glm::vec3>::to_string()
	{
		return core::str::format("%s: [%2.2f, %2.2f, %2.2f]", name, (*data).x, (*data).y, (*data).z);
	}


	void debugvar_register(DebugVarBase* var);
	DebugVarBase* debugvar_first();
} // namespace gemini

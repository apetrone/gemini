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

#include <core/mathlib.h>
#include <core/mem.h>
#include <core/str.h>
#include <core/typedefs.h>


enum ParameterType
{
	Invalid = 0,
	Float,
	Integer,
	Vector3,
	Vector4
};

template <class T>
struct ParameterTypeFromType
{
	static const ParameterType value = ParameterType::Invalid;
};

template <>
struct ParameterTypeFromType<int32_t>
{
	static const ParameterType value = ParameterType::Integer;
};

template <>
struct ParameterTypeFromType<uint32_t>
{
	static const ParameterType value = ParameterType::Integer;
};

template <>
struct ParameterTypeFromType<float>
{
	static const ParameterType value = ParameterType::Float;
};

template <>
struct ParameterTypeFromType<glm::vec3>
{
	static const ParameterType value = ParameterType::Vector3;
};

template <>
struct ParameterTypeFromType<glm::quat>
{
	static const ParameterType value = ParameterType::Vector4;
};

struct Parameter
{
	ParameterType type;
	void* value;
	const char* name;

	template <class T>
	Parameter(const char* display_name, T* val)
		: type(ParameterTypeFromType<T>::value)
		, value(val)
		, name(display_name)
	{
	}
};

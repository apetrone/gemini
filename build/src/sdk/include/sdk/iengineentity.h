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

#include <core/mathlib.h>

namespace gemini
{
	// TODO: Perhaps this should be rolled into another class
	// with a better name. Perhaps the Physics objects?
	struct EntityTransform
	{
		glm::vec3 local_position;
		glm::quat local_orientation;

		glm::mat4 parent_world_transform;
	};


	class IEngineEntity
	{
	public:
		virtual ~IEngineEntity() {};

		// model instance accessors; returns -1 if no model
//		virtual void set_model_index(int32_t index) = 0;
		virtual int32_t get_model_index() const = 0;

		// transform accessors
		virtual EntityTransform get_transform() const = 0;
		virtual void set_transform(const EntityTransform& txform) = 0;
	};
}
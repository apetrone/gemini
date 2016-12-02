// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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

#include "typedefs.h"
#include <core/mathlib.h>

namespace gemini
{
	struct AABB2
	{
		float left;
		float right;
		float top;
		float bottom;

		bool overlaps(const AABB2& other) const;
	}; // AABB2

	struct OrientedBoundingBox
	{
		glm::vec3 center;
		glm::mat3 rotation;
		glm::vec3 positive_extents;
	}; // OrientedBoundingBox

	// A hitbox is SIMILAR to an OBB, but
	// the center and orientation are derived
	// from the target bone.
	struct Hitbox
	{
		glm::mat3 rotation;
		glm::vec3 positive_extents;
	}; // Hitbox

	// compute covariance for a set of values
	float covariance(size_t row, size_t column, float* values, size_t total_values, size_t stride);

	void compute_oriented_bounding_box_by_points(OrientedBoundingBox& box, glm::vec3* vertices, size_t total_vertices);
} // namespace gemini


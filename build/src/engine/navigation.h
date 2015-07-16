// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <core/fixedarray.h>
#include <core/mathlib.h>

#include <sdk/navigation_api.h>

#include <renderer/renderer.h> // for renderer::IndexType

namespace gemini
{
	namespace navigation
	{
		void create_from_geometry(const FixedArray<glm::vec3>& vertices, const FixedArray<::renderer::IndexType>& indices, const glm::vec3& mins, const glm::vec3& maxs);
		
		void startup();
		void shutdown();
		
		void debugdraw();
		
		bool find_poly(NavMeshPolyRef* ref, const glm::vec3& position, const glm::vec3& extents);
		void find_path(NavMeshPath* path, const glm::vec3& start, const glm::vec3& end);
		void debugdraw_path(NavMeshPath* path);
		void find_straight_path(NavMeshPath* path, glm::vec3* positions, uint32_t* total_positions);
		glm::vec3 find_random_location();
	} // namespace navigation
} // namespace gemini

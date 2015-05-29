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

#include <stdint.h>


#include <nom/nom.hpp>
#include "navigation_api.h"

namespace gemini
{
	class IExperimental
	{
	public:
		virtual ~IExperimental();


		virtual gui::Panel* root_panel() const = 0;
		virtual gui::Compositor* get_compositor() const = 0;
		
		virtual void navmesh_find_poly(NavMeshPolyRef* poly, const glm::vec3& position, const glm::vec3& extents) = 0;
		virtual void navmesh_find_path(NavMeshPath* path, const glm::vec3& start, const glm::vec3& end) = 0;
		virtual void navmesh_draw_path(NavMeshPath* path) = 0;
		virtual void navmesh_generate_from_model(const char* path) = 0;
		virtual void navmesh_find_straight_path(NavMeshPath* path, glm::vec3* positions, uint32_t* total_positions) = 0;
	};
} // namespace gemini
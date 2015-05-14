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
#include "common.h"

#include "datamodel/model.h"
#include "datamodel/mesh.h"
#include "datamodel/material.h"

#include "common/extension.h"

#include <platform/mem.h>
#include <core/filesystem.h>
#include <core/stackstring.h>
#include <core/typedefs.h>
#include <core/core.h>
#include <core/logging.h>

using namespace core;

namespace gemini
{
	namespace tools
	{
		void startup()
		{
			platform::startup();

			// setup root path
			StackString<MAX_PATH_SIZE> root_path;
			platform::Result result = platform::get_program_directory(&root_path[0], root_path.max_size());
			core::filesystem::root_directory(&root_path[0], root_path.max_size());

			StackString<MAX_PATH_SIZE> content_path;
			core::filesystem::construct_content_directory(content_path);
			core::filesystem::content_directory(content_path());

			// create default material
			// TODO: move this to a better location one day
			datamodel::Material* material = MEMORY_NEW(datamodel::Material, platform::memory::global_allocator());
			datamodel::set_default_material(material);

			core::startup();
		}
		
		void shutdown()
		{
			purge_registry<datamodel::Model>();
			purge_registry<datamodel::Node>();

			// TODO: move this to a better location one day.
			datamodel::Material* material = &datamodel::get_default_material();
			MEMORY_DELETE(material, platform::memory::global_allocator());
			datamodel::set_default_material(0);
			
			core::shutdown();
			platform::shutdown();
		}
	} // namespace tools

	namespace geometry
	{
		glm::vec3 compute_center_of_mass(datamodel::Mesh* mesh)
		{
			glm::vec3 offset;

			// We need to calculate the min and max bounds of the mesh geometry.
			glm::vec3 mins(FLT_MAX);
			glm::vec3 maxs(-FLT_MAX);

			for (size_t v = 0; v < mesh->vertices.size(); ++v)
			{
				const glm::vec3& vertex = mesh->vertices[v];
				mins.x = glm::min(mins.x, vertex.x);
				mins.y = glm::min(mins.y, vertex.y);
				mins.z = glm::min(mins.z, vertex.z);

				maxs.x = glm::max(maxs.x, vertex.x);
				maxs.y = glm::max(maxs.y, vertex.y);
				maxs.z = glm::max(maxs.z, vertex.z);
			}

			return glm::vec3((maxs.x + mins.x)*0.5f, (maxs.y + mins.y)*0.5f, (maxs.z + mins.z)*0.5f);
		}
	} // namespace geometry
} // namespace gemini
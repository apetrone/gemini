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

#include <core/typedefs.h>
#include <platform/platform.h>

#include <runtime/filesystem.h>
#include <runtime/core.h>
#include <runtime/logging.h>

#include <core/stackstring.h>

using namespace core;
using platform::PathString;

namespace gemini
{
	namespace tools
	{
		void startup(const char* application_name)
		{
			platform::startup();

//			// setup root path
			PathString root_path = platform::get_program_directory();
			PathString content_path = platform::fs_content_directory();
//
//			// create default material
//			// TODO: move this to a better location one day
			datamodel::Material* material = MEMORY_NEW(datamodel::Material, core::memory::global_allocator());
			datamodel::set_default_material(material);

			// startup runtime
			core::startup_filesystem();
			core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
			filesystem->root_directory(root_path);
			filesystem->content_directory(content_path);
			
			PathString application_directory = platform::get_user_application_directory(application_name);
			filesystem->user_application_directory(application_directory);

			core::startup_logging();
		}
		
		void shutdown()
		{
			purge_registry<datamodel::Model>();
			purge_registry<datamodel::Node>();

			// TODO: move this to a better location one day.
			datamodel::Material* material = &datamodel::get_default_material();
			MEMORY_DELETE(material, core::memory::global_allocator());
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
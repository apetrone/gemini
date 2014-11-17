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
#include <gemini/typedefs.h>
#include <gemini/mem.h>
#include <gemini/core.h>

#include <slim/xlog.h>

#include "common.h"

#include "datamodel/model.h"
#include "datamodel/mesh.h"

#include "common/extension.h"

namespace tools
{
	void startup()
	{
		memory::startup();
		core::startup();
	}
	
	void shutdown()
	{
		purge_registry<datamodel::Model>();
		purge_registry<datamodel::Node>();
		
		core::shutdown();
		memory::shutdown();
	}
}

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
}

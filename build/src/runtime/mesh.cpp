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
#include <runtime/mesh.h>

using namespace renderer; // get rid of this!

namespace gemini
{
	Mesh::Mesh(gemini::Allocator& _allocator)
		: allocator(_allocator)
		, geometry(_allocator)
		//, geometry_vn(_allocator)
		, skeleton(_allocator)
		, hitboxes(_allocator)
	{
		is_dirty = true;
		has_skeletal_animation = false;
	} // Mesh

	Mesh::~Mesh()
	{
		for (renderer::Geometry* geo : geometry)
		{
			MEMORY2_DELETE(allocator, geo);
		}
	}

	void Mesh::release()
	{
	} // release

	Joint* Mesh::find_bone_named(const char* name)
	{
		core::StackString<128> target_name(name);

		for (Joint& j : skeleton)
		{
			if (j.name == target_name)
			{
				return &j;
			}
		}
		return 0;
	} // find_bone_named

	//AssetLoadStatus mesh_load_callback(const char* path, AssetLoadState<Mesh>& load_state, const AssetParameters& parameters)
	//{
	//	load_state.asset->path = path;
	//	if (core::util::json_load_with_callback(path, /*mesh_load_from_json*/load_json_model, &load_state, true) == core::util::ConfigLoad_Success)
	//	{
	//		return AssetLoad_Success;
	//	}

	//	return AssetLoad_Failure;
	//} // mesh_load_callback
} // namespace gemini

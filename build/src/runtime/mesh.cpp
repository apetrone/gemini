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
	//void mesh_construct_extension(core::StackString<MAX_PATH_SIZE> & extension)
	//{
	//	extension = ".model";
	//} // mesh_construct_extension

	Geometry::Geometry(gemini::Allocator& allocator)
		: ::renderer::Geometry(allocator)
		, bind_poses(allocator)
		, inverse_bind_poses(allocator)
	{
		//material_id = 0;
		vertex_count = 0;
		index_count = 0;
		attributes = 0;
		vertexbuffer = 0;
		draw_type = DRAW_TRIANGLES;
	}

	Geometry::~Geometry()
	{
		if (this->vertexbuffer)
		{
			driver()->vertexbuffer_destroy(this->vertexbuffer);
		}
	}

	// TODO: 01-21-17: This should be moved to the asset compiler.
	void Geometry::render_setup()
	{
		VertexDescriptor descriptor;

		// setup attributes
		if (attributes == 0)
		{
			// always has at least a position
			descriptor.add(VD_FLOAT3);

			if (!normals.empty())
			{
				ShaderString normals = "normals";
				//				attributes |= find_parameter_mask( normals );
				descriptor.add(VD_FLOAT3);
			}

			if (!colors.empty())
			{
				ShaderString colors = "colors";
				//				attributes |= find_parameter_mask( colors );
				descriptor.add(VD_FLOAT4);
			}

			if (uvs.size() > 0)
			{
				ShaderString uv0 = "uv0";
				//				attributes |= find_parameter_mask( uv0 );
				descriptor.add(VD_FLOAT2);
			}

#if 0
			if (uvs.size() > 1)
			{
				ShaderString uv1 = "uv1";
				//				attributes |= find_parameter_mask( uv1 );
				descriptor.add(VD_FLOAT2);
			}
#endif
			if (!blend_indices.empty() && !blend_weights.empty())
			{
				//					ShaderString hardware_skinning = "hardware_skinning";
					//				attributes |= find_parameter_mask(hardware_skinning);

				//					ShaderString node_transforms = "node_transforms";
					//				attributes |= find_parameter_mask(node_transforms);

				descriptor.add(VD_FLOAT4);
				descriptor.add(VD_FLOAT4);
			}
		}

		if (!this->vertexbuffer)
		{
			this->vertexbuffer = driver()->vertexbuffer_from_geometry(descriptor, this);
		}

		if (!this->is_animated())
		{
			driver()->vertexbuffer_upload_geometry(this->vertexbuffer, /*descriptor, */ this);
		}
	}


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
		assert(0); // TODO: 01-19-17: fix this (meshes)
		//for (assets::Geometry* geo : geometry)
		//{
		//	MEMORY2_DELETE(allocator, geo);
		//}
	}

	void Mesh::release()
	{
	} // release

	void Mesh::prepare_geometry()
	{
		if (!is_dirty)
		{
			return;
		}

		assert(geometry.size() != 0);

		assert(0); // TODO: 01-19-17: fix this (meshes)
		//for (unsigned int geo_id = 0; geo_id < geometry.size(); ++geo_id)
		//{
		//	assets::Geometry* geo = geometry[geo_id];
		//	geo->render_setup();
		//}

		is_dirty = false;
	} // prepare_geometry

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

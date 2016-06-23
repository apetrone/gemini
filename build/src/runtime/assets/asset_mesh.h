// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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

#include <vector>

#include <core/mem.h>
#include <core/stackstring.h>

#include "assets.h"
#include "renderer/renderer.h"

#include <core/fixedarray.h>
#include "keyframechannel.h"
#include "animation.h"

#include <json/json.h>

namespace gemini
{
	namespace assets
	{
		// -------------------------------------------------------------
		// Mesh

		const int MAX_VERTEX_WEIGHTS = 4;

		struct Joint
		{
			// -1 == no parent
			int32_t parent_index;
			int32_t index;
			core::StackString<128> name;

			Joint()
			{
				parent_index = -1;
				index = -1;
			}
		};

		struct Geometry : public ::renderer::Geometry
		{
			core::StackString<128> name;
			unsigned int material_id;
			unsigned int shader_id;

			glm::vec3 mins;
			glm::vec3 maxs;

			Geometry();
			~Geometry();

			// set this geometry up for rendering
			void render_setup();

			// model space to bone space transforms
			FixedArray<glm::mat4> bind_poses;

			// object-space to joint-space transforms
			FixedArray<glm::mat4> inverse_bind_poses;
		}; // Geometry


		struct Mesh : public Asset
		{
			FixedArray<Geometry> geometry;
			FixedArray<Geometry> geometry_vn;
			glm::mat4 world_matrix;

			core::StackString<MAX_PATH_SIZE> path;

			// if this is true, it needs to be re-uploaded to the gpu
			bool is_dirty;

			// true when any geometry has a skeleton loaded
			bool has_skeletal_animation;

			// offset to the center of mass
			glm::vec3 mass_center_offset;

			glm::vec3 aabb_mins;
			glm::vec3 aabb_maxs;

			Mesh();
			~Mesh();
			void reset();

			virtual void release();

			// prepare all geometry
			void prepare_geometry();

			glm::mat4 node_transform;

			Joint* find_bone_named(const char* name);

			// bind pose skeleton
			FixedArray<Joint> skeleton;
		}; // Mesh

		AssetLoadStatus mesh_load_callback( const char * path, Mesh * mesh, const AssetParameters & parameters );
		void mesh_construct_extension( core::StackString<MAX_PATH_SIZE> & extension );

		DECLARE_ASSET_LIBRARY_ACCESSOR(Mesh, AssetParameters, meshes);
	} // namespace assets
} // namespace gemini
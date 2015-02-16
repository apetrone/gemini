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

#include <platform/mem.h>
#include <platform/mem_stl_allocator.h>
#include <core/stackstring.h>

#include "assets.h"
#include "renderer/renderer.h"

#include <core/fixedarray.h>
#include "keyframechannel.h"

//#include "scene_graph.h"

#include <json/json.h>

namespace gemini
{
	namespace assets
	{
		// -------------------------------------------------------------
		// Mesh
		
		struct Geometry : public gemini::renderer::Geometry
		{
			core::StackString<128> name;
			unsigned int material_id;
			unsigned int shader_id;
			
			Geometry();
			~Geometry();
			
			// set this geometry up for rendering
			void render_setup();
			
	//		FixedArray<glm::vec3> untransformed_vertices;
			core::FixedArray<glm::vec3> physics_vertices;
		}; // Geometry
		

		// TEMP struct to test the whole shebang.
		struct AnimationData
		{
			core::FixedArray< KeyframeData<glm::vec3> > scale;
			core::FixedArray< KeyframeData<glm::quat> > rotation;
			core::FixedArray< KeyframeData<glm::vec3> > translation;
			
			core::FixedArray< KeyChannel > track_translate;
			
			// name to index map is the best I can do for now.
			typedef std::map<std::string, size_t> NodeToIndexContainer;
			NodeToIndexContainer node_id_by_name;

			// duration of the full animation, in seconds
	//		float duration_seconds;
			
			float frames_per_second;
			float frame_delay_seconds; // 1.0f / frames_per_second
			
			// total frames in this animation
			uint32_t total_frames;
			uint32_t total_keys;
			
			AnimationData() : frames_per_second(0.0f), frame_delay_seconds(0.0f), total_frames(0), total_keys(0) {}
			
			void get_pose(glm::vec3* positions, glm::quat* orientations, float animation_time_seconds);
		};
		
		const int MAX_VERTEX_WEIGHTS = 4;
		
		struct Bone
		{
			Bone()
			{
				parent_index = -1;
				index = -1;
			}
			~Bone() {}
			
			// name of this bone
			String name;
			
			// local to bone space (vertex to bone)
			glm::mat4 inverse_bind_matrix;
			
			// from bone space to local space
			glm::mat4 bind_matrix;
			
			
			glm::mat4 local_transform;
			glm::mat4 world_transform;
			
			// -1: No parent
			int32_t parent_index;
			
			int32_t index;
		};
		
		
		struct Joint
		{
			int8_t parent_index;
			int8_t index;
			core::StackString<128> name;
			
			Joint()
			{
				parent_index = -1;
				index = -1;
			}
			
			// model space to bone space
			glm::mat4 inverse_bind_matrix;
			
			// from bone space back to model space
			glm::mat4 bind_matrix;
		};

		typedef int8_t BoneIndex;
		
		struct Mesh : public Asset
		{
			core::FixedArray<Geometry> geometry;
			core::FixedArray<Geometry> geometry_vn;
			core::FixedArray<Bone> bones;
			glm::mat4 world_matrix;
					
			core::StackString<MAX_PATH_SIZE> path;
			
//			scenegraph::Node* scene_root;
			
			
			
			
			unsigned short total_bones;
			
			// if this is true, it needs to be re-uploaded to the gpu
			bool is_dirty;
			
			// offset to the center of mass
			glm::vec3 mass_center_offset;
			
			Mesh();
			~Mesh();
			void reset();

			virtual void release();
			
			// prepare all geometry
			void prepare_geometry();
			
			// For now, we only have room for a single animation -- so make it worthwhile.
			AnimationData animation;

			glm::mat4 node_transform;
			
			Joint* find_bone_named(const char* name);
			
			// bind pose skeleton
			core::FixedArray<Joint> skeleton;
		}; // Mesh
		
		// EXPERIMENTAL
		void read_keys_object(AnimationData& anim, Bone* bone, Json::Value& jkeys);
		
		AssetLoadStatus mesh_load_callback( const char * path, Mesh * mesh, const AssetParameters & parameters );
		void mesh_construct_extension( core::StackString<MAX_PATH_SIZE> & extension );
		
		DECLARE_ASSET_LIBRARY_ACCESSOR(Mesh, AssetParameters, meshes);
	} // namespace assets
} // namespace gemini
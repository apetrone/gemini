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
#include <runtime/animation.h>
#include <runtime/configloader.h>
#include <runtime/filesystem.h>
#include <runtime/geometry.h>
#include <runtime/mesh_library.h>
#include <runtime/mesh.h>
#include <runtime/assets.h>

#include <renderer/renderer.h>

#include <core/logging.h>
#include <core/mem.h>

//#include "assets.h" // for materials
//#include "assets/asset_mesh.h"

// for MAX_INFLUENCES_PER_VERTEX
#include <shared/shared_constants.h>

// get rid of these!
#include <map>
#include <string.h>

namespace gemini
{
	typedef std::map<int, std::string> MaterialByIdContainer;

	template <class Type>
	void from_json(Type& output, const Json::Value& input);

	template <>
	void from_json(glm::vec3& output, const Json::Value& input)
	{
		output = glm::vec3(input[0].asFloat(), input[1].asFloat(), input[2].asFloat());
	}

	template <>
	void from_json(glm::quat& output, const Json::Value& input)
	{
		output = glm::quat(input[3].asFloat(), input[0].asFloat(), input[1].asFloat(), input[2].asFloat());
	}

	glm::mat4 json_to_mat4(const Json::Value& value)
	{
		glm::mat4 output;
		Json::ValueIterator it = value.begin();
		assert(value.size() > 0);
		float m[16] = { 0 };
		size_t i = 0;
		for (; it != value.end(); ++it, ++i)
		{
			Json::Value item = *it;
			m[i] = item.asFloat();
		}

		// this is equivalent to indexing m sequentially
		return glm::make_mat4(m);
	}

	struct MeshLoaderState
	{
		size_t current_geometry;
		Mesh* mesh;
		gemini::Allocator& allocator;

		MeshLoaderState(gemini::Allocator& _allocator, Mesh* input)
			: allocator(_allocator)
			, current_geometry(0)
			, mesh(input)
		{
		}
	}; // MeshLoaderState

	void traverse_nodes(MeshLoaderState& state, const Json::Value& node, MaterialByIdContainer& materials)
	{
		const std::string node_type = node["type"].asString();
		if (node_type == "mesh")
		{
	//				Json::Value mesh_root = node["mesh"];
			Json::Value bbox_mins = node["mins"];
			Json::Value bbox_maxs = node["maxs"];
	//				assert(!mesh_root.isNull());

			Json::Value index_array = node["indices"];
			Json::Value vertex_array = node["vertices"];
			Json::Value normal_array = node["normals"];
			Json::Value uv_sets = node["uv_sets"];
			Json::Value vertex_colors = node["vertex_colors"];
			const Json::Value& blend_weights = node["blend_weights"];
			const Json::Value& bind_data = node["bind_data"];

			// load skeleton, if one exists
			// this will only construct the hierarchy -- which should be consistent
			// for a single model file.
			// I can't foresee needing multiple skeletons in the same model just yet.
			Json::Value skeleton = node["skeleton"];
			if (!skeleton.isNull())
			{
				// allocate enough bones
				state.mesh->skeleton.allocate(skeleton.size());

				size_t bone_index = 0;

				Json::ValueIterator it = skeleton.begin();
				for (; it != skeleton.end(); ++it, ++bone_index)
				{
					const Json::Value& skeleton_entry = (*it);
					const Json::Value& name = skeleton_entry["name"];
					const Json::Value& parent = skeleton_entry["parent"];

					Joint& joint = state.mesh->skeleton[bone_index];
					joint.index = bone_index;

					if (!name.isNull())
					{
						joint.name = name.asString().c_str();
					}

					if (!parent.isNull())
					{
						joint.parent_index = parent.asInt();
					}

//					LOGV("read joint: %s, parent: %i, index: %i\n", joint.name(), joint.parent_index, joint.index);
				}
			}

			GeometryDefinition* geometry = &state.mesh->geometry[state.current_geometry];

			// read all geometry data into arrays
			glm::vec3* vertex = &state.mesh->vertices[geometry->vertex_offset];
			glm::vec3* normal = &state.mesh->normals[geometry->vertex_offset];
			glm::vec2* uv = &state.mesh->uvs[geometry->vertex_offset];

			for (uint32_t vertex_index = 0;
				vertex_index < geometry->total_vertices;
				++vertex_index, ++vertex, ++normal, ++uv)
			{
				const Json::Value& in_vertex = vertex_array[vertex_index];
				*vertex = glm::vec3(in_vertex[0].asFloat(), in_vertex[1].asFloat(), in_vertex[2].asFloat());

				const Json::Value& in_normal = normal_array[vertex_index];
				*normal = glm::vec3(in_normal[0].asFloat(), in_normal[1].asFloat(), in_normal[2].asFloat());

				const Json::Value& in_uv = uv_sets[0][vertex_index];
				*uv = glm::vec2(in_uv[0].asFloat(), in_uv[1].asFloat());
			}

			// If you hit this assert, we need to finally split
			// large batches of geometry into batches no larger than 65k.
			assert(geometry->total_indices < USHRT_MAX);

			// read all indices
			index_t* indices = &state.mesh->indices[geometry->index_offset];
			for (uint32_t index = 0; index < geometry->total_indices; ++index)
			{
				indices[index] = geometry->vertex_offset + index_array[index].asInt();
			}

			// assign material to this geometry
			Json::Value material_id = node["material_id"];
			if (!material_id.isNull())
			{
				MaterialByIdContainer::iterator it = materials.find(material_id.asInt());
				if (it != materials.end())
				{
					std::string material_path = it->second;
					geometry->material_handle = material_load(material_path.c_str());
				}
			}
			else
			{
				// no material specified; load default material
				geometry->material_handle = material_load("default");
			}

			// If this has a skeleton...
			if (!state.mesh->skeleton.empty())
			{
				// blend weight array must match the total vertices
				LOGV("blend_weights.size = %i, geometry->total_vertices = %i\n", blend_weights.size(), geometry->total_vertices);
				assert(blend_weights.size() == geometry->total_vertices);

				// load animation data

				// TODO: allocate hitboxes

				Json::ValueIterator it = bind_data.begin();
				for (; it != bind_data.end(); ++it)
				{
					const Json::Value& skeleton_entry = (*it);
					const Json::Value& name = skeleton_entry["name"];

					const std::string& bone_name = name.asString();
					Joint* joint = mesh_find_bone_named(state.mesh, bone_name.c_str());
					assert(joint != 0);

					size_t bone_index = joint->index;
//					LOGV("reading bind_pose for '%s' -> %i\n", bone_name.c_str(), bone_index);

					const Json::Value& bind_pose = skeleton_entry["bind_pose"];
					assert(!bind_pose.isNull());
					state.mesh->bind_poses[bone_index] = json_to_mat4(bind_pose);

					const Json::Value& inverse_bind_pose = skeleton_entry["inverse_bind_pose"];
					assert(!inverse_bind_pose.isNull());
					state.mesh->inverse_bind_poses[bone_index] = json_to_mat4(inverse_bind_pose);
				}

				// read all blend weights and indices
				it = blend_weights.begin();
				for (size_t weight_id = 0; it != blend_weights.end(); ++it, ++weight_id)
				{
					const Json::Value& weight_pairs = (*it);

					size_t blend_index = 0;

					int bone_indices[MAX_INFLUENCES_PER_VERTEX];
					memset(bone_indices, 0, sizeof(int) * MAX_INFLUENCES_PER_VERTEX);

					float bone_weights[MAX_INFLUENCES_PER_VERTEX];
					memset(bone_weights, 0, sizeof(float) * MAX_INFLUENCES_PER_VERTEX);

					Json::ValueIterator pair = weight_pairs.begin();
					const size_t total_influences = weight_pairs.size();
					assert(total_influences <= MAX_INFLUENCES_PER_VERTEX);

					for (; pair != weight_pairs.end(); ++pair, ++blend_index)
					{
						const Json::Value& weightblock = (*pair);
						const Json::Value& bone = weightblock["bone"];
						const Json::Value& value = weightblock["value"];
						Joint* joint = mesh_find_bone_named(state.mesh, bone.asString().c_str());
						assert(joint != 0);

//						LOGV("[%i] bone: '%s', value: %2.2f\n", weight_id, bone.asString().c_str(), value.asFloat());

						bone_indices[blend_index] = joint->index;
						bone_weights[blend_index] = value.asFloat();
					}

					// assign these values to the blend_weights and blend_indices index
					state.mesh->blend_indices[weight_id] = glm::vec4(bone_indices[0], bone_indices[1], bone_indices[2], bone_indices[3]);
					state.mesh->blend_weights[weight_id] = glm::vec4(bone_weights[0], bone_weights[1], bone_weights[2], bone_weights[3]);

					//LOGV("[%i] indices: %i %i %i %i\n", weight_id, bone_indices[0], bone_indices[1], bone_indices[2], bone_indices[3]);
					//LOGV("[%i] weights: %2.2f %2.2f %2.2f %2.2f\n", weight_id, bone_weights[0], bone_weights[1], bone_weights[2], bone_weights[3]);
				}


				//Hitbox hb;
				//hb.positive_extents = glm::vec3(0.25f, 0.25f, 0.25f);
				//for (size_t index = 0; index < state.mesh->skeleton.size(); ++index)
				//{
				//	const Joint& joint = state.mesh->skeleton[index];
				//	Hitbox& hitbox = state.mesh->hitboxes[joint.index];
				//	//hitbox.positive_extents = glm::vec3(0.25f, 0.25f, 0.25f);
				//	//hitbox.rotation = glm::toMat3(glm::angleAxis(glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
				//	hitbox = hb;
				//}

				if (!bbox_mins.isNull() && !bbox_maxs.isNull())
				{
					state.mesh->aabb_mins = glm::vec3(bbox_mins[0].asFloat(), bbox_mins[1].asFloat(), bbox_mins[2].asFloat());
					state.mesh->aabb_maxs = glm::vec3(bbox_maxs[0].asFloat(), bbox_maxs[1].asFloat(), bbox_maxs[2].asFloat());
				}

				// physics related settings
				const Json::Value& center_mass_offset = node["mass_center_offset"];
				if (!center_mass_offset.isNull())
				{
					state.mesh->mass_center_offset = glm::vec3(center_mass_offset[0].asFloat(), center_mass_offset[1].asFloat(), center_mass_offset[2].asFloat());
				}
			}
			++state.current_geometry;
		}
		else
		{
			LOGV("create node type: '%s'\n", node_type.c_str());
		}

		const Json::Value& children = node["children"];
		if (!children.isNull())
		{
			Json::ValueIterator child_iter = children.begin();
			for (; child_iter != children.end(); ++child_iter)
			{
				traverse_nodes(state, (*child_iter), materials);
			}
		}
	}

	void count_nodes(const Json::Value& node, size_t& total_nodes, size_t& total_meshes)
	{
		assert(!node["name"].isNull());
		assert(!node["type"].isNull());
		std::string node_name = node["name"].asString();
	//		LOGV("node %s\n", node_name.c_str());

		++total_nodes;

		std::string node_type = node["type"].asString();
		if (node_type == "mesh")
		{
			++total_meshes;
		}

		const Json::Value& children = node["children"];
		if (!children.isNull())
		{
			Json::ValueIterator child_iter = children.begin();
			for (; child_iter != children.end(); ++child_iter)
			{
				count_nodes((*child_iter), total_nodes, total_meshes);
			}
		}
	}

	struct SceneInfo
	{
		Array<GeometryDefinition> geometry;
		uint32_t current_vertex_offset;
		uint32_t current_index_offset;
		uint32_t total_bones;

		SceneInfo(Allocator& allocator)
			: geometry(allocator)
			, current_vertex_offset(0)
			, current_index_offset(0)
			, total_bones(0)
		{
		}
	};



	void collect_scene_data(const Json::Value& node, SceneInfo* scene_info)
	{
		std::string node_type = node["type"].asString();
		if (node_type == "mesh")
		{
			Json::Value index_array = node["indices"];
			Json::Value vertex_array = node["vertices"];
			Json::Value normal_array = node["normals"];

			const size_t total_vertices = vertex_array.size();
			const size_t total_indices = index_array.size();

			GeometryDefinition geometry_definition;
			geometry_definition.material_handle = InvalidAssetHandle;
			geometry_definition.shader_handle = InvalidAssetHandle;
			geometry_definition.vertex_offset = scene_info->current_vertex_offset;
			geometry_definition.index_offset = scene_info->current_index_offset;
			geometry_definition.total_vertices = total_vertices;
			geometry_definition.total_indices = total_indices;

			scene_info->current_vertex_offset += total_vertices;
			scene_info->current_index_offset += total_indices;

			scene_info->geometry.push_back(geometry_definition);

			// load skeleton, if one exists
			// this will only construct the hierarchy -- which should be consistent
			// for a single model file.
			// I can't foresee needing multiple skeletons in the same model just yet.
			Json::Value skeleton = node["skeleton"];
			scene_info->total_bones = skeleton.size();
		}

		const Json::Value& children = node["children"];
		if (!children.isNull())
		{
			Json::ValueIterator child_iter = children.begin();
			for (; child_iter != children.end(); ++child_iter)
			{
				collect_scene_data((*child_iter), scene_info);
			}
		}
	} // collect_scene_Data


	core::util::ConfigLoadStatus load_json_model(const Json::Value& root, void* data)
	{
		MeshLibrary::LoadState* load_state = reinterpret_cast<MeshLibrary::LoadState*>(data);
		Mesh* mesh = load_state->asset;

		Json::Value node_root = root["children"];

		// The model has no nodes. What have you done?
		assert(!node_root.isNull());

		MaterialByIdContainer materials_by_id;

		// read in materials
		Json::Value materials = root["materials"];
		if (!materials.isNull())
		{
			Json::ValueIterator miter = materials.begin();
			for (; miter != materials.end(); ++miter)
			{
				Json::Value material = (*miter);
				materials_by_id.insert(MaterialByIdContainer::value_type(material["id"].asInt(), material["name"].asString()));
			}
		}

		SceneInfo scene_info(*load_state->allocator);
		{
			Json::ValueIterator node_iter = node_root.begin();
			for (; node_iter != node_root.end(); ++node_iter)
			{
				Json::Value node = (*node_iter);
				collect_scene_data(node, &scene_info);
			}
		}

		mesh->geometry.allocate(scene_info.geometry.size());
		memcpy(&mesh->geometry[0], &scene_info.geometry[0], sizeof(GeometryDefinition) * scene_info.geometry.size());

		mesh_init(*load_state->allocator, mesh, scene_info.current_vertex_offset, scene_info.current_index_offset, scene_info.total_bones);

		MeshLoaderState state(*load_state->allocator, mesh);

		// traverse over hierarchy
		Json::ValueIterator node_iter = node_root.begin();
		for (; node_iter != node_root.end(); ++node_iter)
		{
			Json::Value node = (*node_iter);
			traverse_nodes(state, node, materials_by_id);
		}

		// try to load animations
		Json::Value animation_list = root["animations"];
		if (!animation_list.isNull())
		{
			Json::ValueIterator it = animation_list.begin();

			uint32_t total_sequences = animation_list.size();
			mesh->sequences.allocate(total_sequences);

			uint32_t animation_index = 0;
			for (; it != animation_list.end(); ++it, ++animation_index)
			{
				const Json::Value& animation_name = (*it);
				std::string name = animation_name.asString();

				platform::PathString animation_sequence_uri = load_state->asset_uri.dirname();
				animation_sequence_uri.append(PATH_SEPARATOR_STRING);
				animation_sequence_uri.append(name.c_str());
				animation::Sequence* sequence = animation::load_sequence_from_file(*load_state->allocator, animation_sequence_uri(), mesh);
				mesh->sequences[animation_index] = sequence->index;

				platform::PathString basename = animation_sequence_uri.basename();
				assert(basename.size() < 32);
				mesh->sequence_index_by_name[basename()] = animation_index;
			}
		}

		// try loading embedded collision geometry
		Json::Value collision_geometry = root["collision_geometry"];
		if (!collision_geometry.isNull())
		{
			Json::Value collision_vertices = collision_geometry["vertices"];
			Json::Value collision_normals = collision_geometry["normals"];
			Json::Value collision_indices = collision_geometry["indices"];

			// Sanity check that vertices are 1:1 with normals.
			assert(collision_vertices.size() == collision_normals.size());

			// If you hit this assert, we need to finally split
			// large batches of geometry into batches no larger than 65k.
			// As a collision mesh, this limit should never be hit, question mark?
			assert(collision_indices.size() < USHRT_MAX);

			LOGV("> Load mesh collision: verts: %i, indices: %i\n", collision_vertices.size(), collision_indices.size());

			mesh_create_collision(*load_state->allocator, mesh, collision_vertices.size(), collision_indices.size());

			// read all vertices and normals
			for (uint32_t vertex = 0; vertex < collision_vertices.size(); ++vertex)
			{
				glm::vec3& out_vertex = mesh->collision_geometry->vertices[vertex];
				glm::vec3& out_normal = mesh->collision_geometry->normals[vertex];

				const Json::Value& in_vertex = collision_vertices[vertex];
				out_vertex = glm::vec3(in_vertex[0].asFloat(), in_vertex[1].asFloat(), in_vertex[2].asFloat());

				const Json::Value& in_normal = collision_normals[vertex];
				out_normal = glm::vec3(in_normal[0].asFloat(), in_normal[1].asFloat(), in_normal[2].asFloat());
			}

			// read all indices
			index_t* indices = mesh->collision_geometry->indices;
			for (uint32_t index = 0; index < collision_indices.size(); ++index)
			{
				indices[index] = collision_indices[index].asInt();
			}

		}

		return core::util::ConfigLoad_Success;
	}


	MeshLibrary::MeshLibrary(Allocator& allocator)
		: AssetLibrary2(allocator)
	{
	}

	void MeshLibrary::create_asset(LoadState& state, void* parameters)
	{
		state.asset = MEMORY2_NEW(*state.allocator, Mesh)(*state.allocator);
	}

	AssetLoadStatus MeshLibrary::load_asset(LoadState& state, platform::PathString& fullpath, void* parameters)
	{
		LOGV("loading mesh \"%s\"\n", fullpath());

		platform::PathString asset_uri = fullpath;
		asset_uri.append(".model");

		if (core::util::json_load_with_callback(asset_uri(), load_json_model, &state, true) == core::util::ConfigLoad_Success)
		{
			return AssetLoad_Success;
		}

		return AssetLoad_Failure;
	}

	void MeshLibrary::destroy_asset(LoadState& state)
	{
		mesh_destroy(*state.allocator, state.asset);

		MEMORY2_DELETE(*state.allocator, state.asset);
	}
} // namespace gemini

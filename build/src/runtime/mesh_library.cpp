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
#include <runtime/configloader.h>
#include <runtime/filesystem.h>
#include <runtime/geometry.h>
#include <runtime/mesh_library.h>
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

	void traverse_nodes(MeshLoaderState& state, const Json::Value& node, MaterialByIdContainer& materials, const bool is_world)
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

	//						LOGV("read joint: %s, parent: %i, index: %i\n", joint.name(), joint.parent_index, joint.index);
				}
			}


			// setup materials

			Geometry* geo = MEMORY2_NEW(state.allocator, Geometry)(state.allocator);
			//assets::Geometry* geo = state.mesh->geometry[state.current_geometry++];
			state.mesh->geometry[state.current_geometry++] = geo;

			geo->material_id = material_load("materials/default");

			Json::Value material_id = node["material_id"];
			if (!material_id.isNull())
			{
				// assign this material
				auto it = materials.find(material_id.asInt());
				if (it != materials.end())
				{
					std::string material_name = it->second;
					//if (is_world)
					//{
					//	material_name.append("_world");
					//}

					std::string material_path = "materials/" + material_name;
					AssetHandle material_handle = material_load(material_path.c_str());
					geo->material_id = material_handle;
				}
			}

			// TODO: Remove this hack in the rewrite
			std::string shader_path = "objects";
			if (!bind_data.empty())
			{
				shader_path = "animation";
			}

			geo->shader_id = shader_load(shader_path.c_str());
			geo->draw_type = renderer::DRAW_INDEXED_TRIANGLES;
			geo->name = node["name"].asString().c_str();

	//				LOGV("assign material id %i to geometry: %s\n", geo->material_id, geo->name());
	//				LOGV("load geometry: %s\n", geo->name());

			// allocate the geometry arrays
			geo->index_count = index_array.size();
			geo->vertex_count = vertex_array.size();
			geo->indices.allocate(index_array.size());
			geo->vertices.allocate(vertex_array.size());
			geo->normals.allocate(vertex_array.size());
			geo->uvs.allocate(vertex_array.size() * renderer::GEOMETRY_UV_SET_MAX);
			geo->colors.allocate(vertex_colors.size());

			if (!bind_data.empty())
			{
				geo->blend_weights.allocate(geo->vertex_count);
				geo->blend_indices.allocate(geo->vertex_count);
			}

			// read all indices
			for (int i = 0; i < static_cast<int>(geo->indices.size()); ++i)
			{
				geo->indices[i] = index_array[i].asInt();
			}

			// read vertices and normals
			for (int v = 0; v < static_cast<int>(geo->vertices.size()); ++v)
			{
				const Json::Value& vertex = vertex_array[v];
				geo->vertices[v] = glm::vec3(vertex[0].asFloat(), vertex[1].asFloat(), vertex[2].asFloat());
	//					LOGV("v: %i | %g %g %g\n", v, geo->vertices[v].x, geo->vertices[v].y, geo->vertices[v].z);

				const Json::Value& normal = normal_array[v];
				geo->normals[v] = glm::vec3(normal[0].asFloat(), normal[1].asFloat(), normal[2].asFloat());
			}

			// read vertex colors
			for (int v = 0; v < static_cast<int>(geo->colors.size()); ++v)
			{
				const Json::Value& vertex_color = vertex_colors[v];
				geo->colors[v] = Color(vertex_color[0].asFloat(), vertex_color[1].asFloat(), vertex_color[2].asFloat(), vertex_color[3].asFloat());
			}

			// read uv sets
			for (Json::Value::ArrayIndex set_id = 0; set_id < uv_sets.size(); ++set_id)
			{
				//geo->uvs[set_id].allocate(vertex_array.size());
				assert(vertex_array.size() == geo->vertices.size());
				for (int v = 0; v < static_cast<int>(geo->vertices.size()); ++v)
				{
					const Json::Value& texcoord = uv_sets[set_id][v];
					glm::vec2& uv = geo->uvs[(v * renderer::GEOMETRY_UV_SET_MAX) + set_id];
					uv.x = texcoord[0].asFloat();
					uv.y = texcoord[1].asFloat();
	//						LOGV("uv (set=%i) (vertex=%i) %g %g\n", set_id, v, uv.s, uv.t);
				}
			}


			if (!blend_weights.empty())
			{
	//					LOGV("blend_weights.size = %i, geo->vertex_count = %i\n", blend_weights.size(), geo->vertex_count);
				assert(blend_weights.size() == geo->vertex_count);
			}


			if (!bind_data.isNull() && !bind_data.empty())
			{
				// allocate enough bones
				geo->bind_poses.allocate(bind_data.size());
				geo->inverse_bind_poses.allocate(bind_data.size());
				state.mesh->hitboxes.allocate(bind_data.size());

				Json::ValueIterator it = bind_data.begin();
				for (; it != bind_data.end(); ++it)
				{
					const Json::Value& skeleton_entry = (*it);
					const Json::Value& name = skeleton_entry["name"];

					const std::string& bone_name = name.asString();
					Joint* joint = state.mesh->find_bone_named(bone_name.c_str());
					assert(joint != 0);

					size_t bone_index = joint->index;
					//						LOGV("reading bind_pose for '%s' -> %i\n", bone_name.c_str(), bone_index);
					const Json::Value& inverse_bind_pose = skeleton_entry["inverse_bind_pose"];
					assert(!inverse_bind_pose.isNull());
					geo->inverse_bind_poses[bone_index] = json_to_mat4(inverse_bind_pose);

					const Json::Value& bind_pose = skeleton_entry["bind_pose"];
					assert(!bind_pose.isNull());
					geo->bind_poses[bone_index] = json_to_mat4(bind_pose);
				}


				state.mesh->has_skeletal_animation = true;


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

						Joint* joint = state.mesh->find_bone_named(bone.asString().c_str());
						assert(joint != 0);

	//							LOGV("[%i] bone: '%s', value: %2.2f\n", weight_id, bone.asString().c_str(), value.asFloat());

						bone_indices[blend_index] = joint->index;
						bone_weights[blend_index] = value.asFloat();
					}

					// assign these values to the blend_weights and blend_indices index
					geo->blend_indices[weight_id] = glm::vec4(bone_indices[0], bone_indices[1], bone_indices[2], bone_indices[3]);
	//						LOGV("[%i] indices: %g %g %g %g\n", weight_id, geo->blend_indices[weight_id].x, geo->blend_indices[weight_id].y, geo->blend_indices[weight_id].z, geo->blend_indices[weight_id].w);
					geo->blend_weights[weight_id] = glm::vec4(bone_weights[0], bone_weights[1], bone_weights[2], bone_weights[3]);
	//						LOGV("weights: %2.2f %2.2f %2.2f %2.f\n", geo->blend_weights[weight_id].x, geo->blend_weights[weight_id].y, geo->blend_weights[weight_id].z, geo->blend_weights[weight_id].w);
				}

				Hitbox hb;
				hb.positive_extents = glm::vec3(0.25f, 0.25f, 0.25f);


				for (size_t index = 0; index < state.mesh->skeleton.size(); ++index)
				{
					const Joint& joint = state.mesh->skeleton[index];
					Hitbox& hitbox = state.mesh->hitboxes[joint.index];
					//hitbox.positive_extents = glm::vec3(0.25f, 0.25f, 0.25f);
					//hitbox.rotation = glm::toMat3(glm::angleAxis(glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
					hitbox = hb;
				}
			}


			if (!bbox_mins.isNull() && !bbox_maxs.isNull())
			{
				state.mesh->aabb_mins = geo->mins = glm::vec3(bbox_mins[0].asFloat(), bbox_mins[1].asFloat(), bbox_mins[2].asFloat());
				state.mesh->aabb_maxs = geo->maxs = glm::vec3(bbox_maxs[0].asFloat(), bbox_maxs[1].asFloat(), bbox_maxs[2].asFloat());
			}

			// physics related settings
			const Json::Value& center_mass_offset = node["mass_center_offset"];
			if (!center_mass_offset.isNull())
			{
				state.mesh->mass_center_offset = glm::vec3(center_mass_offset[0].asFloat(), center_mass_offset[1].asFloat(), center_mass_offset[2].asFloat());
			}
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
				traverse_nodes(state, (*child_iter), materials, false /*is_world*/);
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

	core::util::ConfigLoadStatus load_json_model(const Json::Value& root, void* data)
	{
		MeshLibrary::LoadState* load_state = reinterpret_cast<MeshLibrary::LoadState*>(data);
		Mesh* mesh = load_state->asset;

		// TODO: remove this hack for maps in the re-write.
		//core::StackString<MAX_PATH_SIZE> fullpath = mesh->path;
		//bool is_world = fullpath.startswith("maps");

		// make sure we can load the default material
		//Material* default_mat = materials()->default_asset();
		//if (!default_mat)
		//{
		//	LOGE("Could not load the default material!\n");
		//	return core::util::ConfigLoad_Failure;
		//}

		// n-meshes
		// skeleton (should this be separate?)
		// animation (should be separate)

		// try to read all geometry
		// first pass will examine nodes

		//LOGV("loading model '%s'...\n", mesh->path());

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

		size_t total_nodes = 0;
		size_t total_meshes = 0;

		// iterate over all nodes and count how many there are, of each kind
		Json::ValueIterator node_iter = node_root.begin();
		for (; node_iter != node_root.end(); ++node_iter)
		{
			Json::Value node = (*node_iter);
			count_nodes(node, total_nodes, total_meshes);
		}

		// allocate nodes
		LOGV("nodes: %i, meshes: %i\n", total_nodes, total_meshes);
		mesh->geometry.allocate(total_meshes);



		MeshLoaderState state(*load_state->allocator, mesh);

		// traverse over hierarchy
		node_iter = node_root.begin();
		for (; node_iter != node_root.end(); ++node_iter)
		{
			Json::Value node = (*node_iter);
			traverse_nodes(state, node, materials_by_id, false /*is_world*/);
		}


		return core::util::ConfigLoad_Success;
	}


	MeshLibrary::MeshLibrary(Allocator& allocator, render2::Device* render_device)
		: AssetLibrary2(allocator)
		, device(render_device)
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
			for (size_t index = 0; index < state.asset->geometry.size(); ++index)
			{
				Geometry* geo = state.asset->geometry[index];
				LOGV("geo [%i] vertices: %i\n", index, geo->vertex_count);
				geo->render_setup();


			}
			return AssetLoad_Success;
		}



		return AssetLoad_Failure;
	}

	void MeshLibrary::destroy_asset(LoadState& state)
	{
		MEMORY2_DELETE(*state.allocator, state.asset);
	}
} // namespace gemini

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
#include <core/configloader.h>
#include <core/logging.h>

#include "assets.h"
#include "assets/asset_mesh.h"
#include "assets/asset_material.h"
#include "renderer/renderer.h"

//#include "skeletalnode.h"

using namespace gemini::renderer;

namespace gemini
{
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

	namespace assets
	{
		typedef std::map<int, std::string> MaterialByIdContainer;

		static glm::mat4 json_to_mat4(const Json::Value& value)
		{
			glm::mat4 output;
			Json::ValueIterator it = value.begin();
			assert(value.size() > 0);
			float m[16] = {0};
			size_t i = 0;
			for( ; it != value.end(); ++it, ++i )
			{
				Json::Value item = *it;
				m[i] = item.asFloat();
			}
			
			return glm::mat4(
				m[0], m[1], m[2], m[3],
				m[4], m[5], m[6], m[7],
				m[8], m[9], m[10], m[11],
				m[12], m[13], m[14], m[15]
			);
		}

		// -------------------------------------------------------------
		// Mesh
		
		struct MeshLoaderState
		{
			size_t current_geometry;
			Mesh* mesh;
			BoneIndex parent;
			
			MeshLoaderState(Mesh* input) : current_geometry(0), mesh(input), parent(-1) {}
		};
		
		
//		void AnimationData::get_pose(glm::vec3 *positions, glm::quat *rotations, float animation_time_seconds)
//		{
//			for (uint32_t index = 0; index < total_bones; ++index)
//			{
//				positions[index] = track_translate[index].value_at_time(animation_time_seconds);
//				rotations[index] = track_rotation[index].value_at_time(animation_time_seconds);
//			}
//		}
		
		void traverse_nodes(MeshLoaderState& state, const Json::Value& node, MaterialByIdContainer& materials, const bool is_world)
		{
			std::string node_type = node["type"].asString();
//			scenegraph::Node* node_root = nullptr;
			
			if (node_type == "mesh")
			{
			
				Json::Value mesh_root = node["mesh"];
				assert(!mesh_root.isNull());

				Json::Value index_array = mesh_root["indices"];
				Json::Value vertex_array = mesh_root["vertices"];
				Json::Value normal_array = mesh_root["normals"];
				Json::Value uv_sets = mesh_root["uv_sets"];
				Json::Value vertex_colors = mesh_root["vertex_colors"];
				const Json::Value& blend_weights = mesh_root["blend_weights"];
				
				// setup materials
				assets::Material* default_material = assets::materials()->get_default();
				
				assets::Geometry* geo = &state.mesh->geometry[state.current_geometry++];
				geo->material_id = default_material->Id();
				
				Json::Value material_id = mesh_root["material_id"];
				if (!material_id.isNull())
				{
					// assign this material
					auto it = materials.find(material_id.asInt());
					if (it != materials.end())
					{
						std::string material_name = it->second;
						if (is_world)
						{
							material_name.append("_world");
						}
						Material* material = assets::materials()->load_from_path(material_name.c_str());
						geo->material_id = material->Id();
					}
				}
				
				// TODO: Remove this hack in the rewrite
				std::string shader_path = "shaders/objects";
				if (is_world)
				{
					shader_path = "shaders/world";
				}
				else if (!state.mesh->skeleton.empty())
				{
					shader_path = "shaders/animation";
				}
				
				assets::Shader* shader = assets::shaders()->load_from_path(shader_path.c_str());
				geo->shader_id = shader->Id();
				geo->draw_type = renderer::DRAW_INDEXED_TRIANGLES;
				geo->name = node["name"].asString().c_str();
				
//				LOGV("assign material id %i to geometry: %s\n", geo->material_id, geo->name());
	//			LOGV("load geometry: %s\n", geo->name());

				// allocate the geometry arrays
				geo->index_count = index_array.size();
				geo->vertex_count = vertex_array.size();
				geo->indices.allocate(index_array.size());
				geo->vertices.allocate(vertex_array.size());
				geo->normals.allocate(vertex_array.size());
				geo->uvs.allocate(uv_sets.size());
				geo->colors.allocate(vertex_colors.size());

				geo->blend_weights.allocate(geo->vertex_count);
				geo->blend_indices.allocate(geo->vertex_count);

				// read all indices
				for (int i = 0; i < geo->indices.size(); ++i)
				{
					geo->indices[i] = index_array[i].asInt();
				}
				
				// read vertices and normals
				for (int v = 0; v < geo->vertices.size(); ++v)
				{
					const Json::Value& vertex = vertex_array[v];
					geo->vertices[v] = glm::vec3(vertex[0].asFloat(), vertex[1].asFloat(), vertex[2].asFloat());
	//				LOGV("v: %i | %g %g %g\n", v, geo->vertices[v].x, geo->vertices[v].y, geo->vertices[v].z);
					
					const Json::Value& normal = normal_array[v];
					geo->normals[v] = glm::vec3(normal[0].asFloat(), normal[1].asFloat(), normal[2].asFloat());
				}

				// read vertex colors
				for (int v = 0; v < geo->colors.size(); ++v)
				{
					const Json::Value& vertex_color = vertex_colors[v];
					geo->colors[v] = core::Color(255 * vertex_color[0].asFloat(), 255 * vertex_color[1].asFloat(), 255 * vertex_color[2].asFloat(), 255 * vertex_color[3].asFloat());
				}
				
				// read uv sets
				for (int set_id = 0; set_id < uv_sets.size(); ++set_id)
				{
					geo->uvs[set_id].allocate(vertex_array.size());
					assert(vertex_array.size() == geo->vertices.size());
					for (int v = 0; v < geo->vertices.size(); ++v)
					{
						const Json::Value& texcoord = uv_sets[set_id][v];
						glm::vec2& uv = geo->uvs[set_id][v];
						uv.x = texcoord[0].asFloat();
						uv.y = 1.0 - texcoord[1].asFloat();
						//LOGV("uv (set=%i) (vertex=%i) %g %g\n", set_id, v, uv.s, uv.t);
					}
				}
				

				if (!blend_weights.empty())
				{
//					LOGV("blend_weights.size = %i, geo->vertex_count = %i\n", blend_weights.size(), geo->vertex_count);
					assert(blend_weights.size() == geo->vertex_count);
				}

				// read all blend weights and indices
				Json::ValueIterator it = blend_weights.begin();
				for (int weight_id = 0; it != blend_weights.end(); ++it, ++weight_id)
				{
					const Json::Value& weight_pairs = (*it);
//					LOGV("size: %i\n", weight_pairs.size());
					
					
					int blend_index = 0;

					int bone_indices[4] = {0, 0, 0, 0};
					float bone_weights[4] = {0, 0, 0, 0};
					
					Json::ValueIterator pair = weight_pairs.begin();
					for (; pair != weight_pairs.end(); ++pair, ++blend_index)
					{
						const Json::Value& weightblock = (*pair);
						const Json::Value& bone = weightblock["bone"];
						const Json::Value& value = weightblock["value"];
						
						Joint* joint = state.mesh->find_bone_named(bone.asString().c_str());
						assert(joint != 0);
						
						// TODO: get index of this bone
//						LOGV("bone: %s, value: %2.2f\n", bone.asString().c_str(), value.asFloat());
						
						
						bone_indices[blend_index] = joint->index;
						bone_weights[blend_index] = value.asFloat();
					}
					
					// assign these values to the blend_weights and blend_indices index
					geo->blend_indices[weight_id] = glm::vec4(bone_indices[0], bone_indices[1], bone_indices[2], bone_indices[3]);
//					LOGV("indices: %g %g %g %g\n", geo->blend_indices[weight_id].x, geo->blend_indices[weight_id].y, geo->blend_indices[weight_id].z, geo->blend_indices[weight_id].w);
					geo->blend_weights[weight_id] = glm::vec4(bone_weights[0], bone_weights[1], bone_weights[2], bone_weights[3]);
//					LOGV("weights: %g %g %g %g\n", geo->blend_weights[weight_id].x, geo->blend_weights[weight_id].y, geo->blend_weights[weight_id].z, geo->blend_weights[weight_id].w);
				}
				
				
				// physics related settings
				const Json::Value& center_mass_offset = mesh_root["mass_center_offset"];
				if (!center_mass_offset.isNull())
				{
					state.mesh->mass_center_offset = glm::vec3(center_mass_offset[0].asFloat(), center_mass_offset[1].asFloat(), center_mass_offset[2].asFloat());
				}
			}
//			else if (node_type == "skeleton")
//			{
//				LOGV("create skeleton\n");
////				node_root = CREATE(scenegraph::AnimatedNode);
//
//				// insert the new joint
//				BoneIndex parent_index = state.parent;
//				Joint& joint = state.mesh->skeleton[++state.parent];
//				joint.parent_index = parent_index;
//				joint.index = state.parent;
//				
//				
//				const Json::Value& skeleton_root = node["name"];
//				joint.name = skeleton_root.asString().c_str();
//				LOGV("read joint: %s, parent: %i, index: %i\n", joint.name(), joint.parent_index, joint.index);
//			}
			else
			{
				LOGV("create group node\n");
//				node_root = CREATE(scenegraph::AnimatedNode);
			}
			
//			assert(node_root != nullptr);

//			node_root->name = node["name"].asString().c_str();

			const Json::Value& scaling = node["scaling"];
			const Json::Value& rotation = node["rotation"];
			const Json::Value& translation = node["translation"];
			assert(!scaling.isNull() && !rotation.isNull() && !translation.isNull());


//			from_json(node_root->local_scale, scaling);
//			from_json(node_root->local_rotation, rotation);
//			from_json(node_root->local_position, translation);
	//		LOGV("scale: %g %g %g\n", node_root->local_scale.x, node_root->local_scale.y, node_root->local_scale.z);
	//		LOGV("rotation: %g %g %g %g\n", node_root->local_rotation.x, node_root->local_rotation.y, node_root->local_rotation.z, node_root->local_rotation.w);
	//		LOGV("translation: %g %g %g\n", node_root->local_position.x, node_root->local_position.y, node_root->local_position.z);
		
			const Json::Value& children = node["children"];
			if (!children.isNull())
			{
				Json::ValueIterator child_iter = children.begin();
				for (; child_iter != children.end(); ++child_iter)
				{
					traverse_nodes(state, (*child_iter), materials, is_world);
				}
			}
		}
		
		void count_nodes(const Json::Value& node, size_t& total_nodes, size_t& total_meshes, size_t& total_bones)
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
//			else if (node_type == "skeleton")
//			{
//				++total_bones;
//			}
			
			const Json::Value& children = node["children"];
			if (!children.isNull())
			{
				Json::ValueIterator child_iter = children.begin();
				for (; child_iter != children.end(); ++child_iter)
				{
					count_nodes((*child_iter), total_nodes, total_meshes, total_bones);
				}
			}
		}
		
#if 0
		template <class Type>
		void read_channel(KeyChannel<Type>& channel, const Json::Value& animation_channel)
		{
			if (animation_channel.isNull())
			{
				return;
			}
			const Json::Value& times = animation_channel["time"];
			const Json::Value& values = animation_channel["value"];
			
			if ((times.isNull() || values.isNull()) || (times.empty() && values.empty()))
			{
				return;
			}
			
			assert(times.size() == values.size());
			channel.keys.allocate(times.size());
			
			// Cannot use size_t because jsoncpp raises an 'ambiguous type' error.
			for (int index = 0; index < channel.keys.size(); ++index)
			{
				const Json::Value& time_data = times[index];
				const Json::Value& value_data = values[index];
				
				Keyframe<Type>& keyframe = channel.keys[index];
				keyframe.seconds = time_data.asFloat();
				from_json(keyframe.value, value_data);
			}
		}
			
		template <class Type>
		void read_channel(KeyframeData<Type>& channel, const Json::Value& animation_channel)
		{
			const Json::Value& times = animation_channel["time"];
			const Json::Value& values = animation_channel["value"];

			assert(!times.empty());
			assert(!values.empty());
			
			assert(times.size() == values.size());
			
			channel.time.allocate(times.size());
			channel.keys.allocate(values.size());
			
			for (int key = 0; key < times.size(); ++key)
			{
				const Json::Value& time_data = times[key];
				channel.time[key] = time_data.asFloat();
				
				const Json::Value& value_data = values[key];
				from_json(channel.keys[key], value_data);
			}
		}
#endif
		
		core::util::ConfigLoadStatus load_json_model(const Json::Value& root, void* data)
		{
			Mesh* mesh = (Mesh*)data;

			// TODO: remove this hack for maps in the re-write.
			core::StackString<MAX_PATH_SIZE> fullpath = mesh->path;
			bool is_world = fullpath.startswith("maps");
			
			// make sure we can load the default material
			assets::Material* default_mat = assets::materials()->load_from_path("materials/default");
			if (!default_mat)
			{
				LOGE("Could not load the default material!\n");
				return core::util::ConfigLoad_Failure;
			}
			
//			mesh->scene_root = CREATE(scenegraph::Node);
//			mesh->scene_root->name = mesh->path();
			
			// n-meshes
			// skeleton (should this be separate?)
			// animation (should be separate)
			
			// try to read all geometry
			// first pass will examine nodes
					
			Json::Value node_root = root["nodes"];
			
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
			size_t total_bones = 0;


			// load skeleton, if one exists
			Json::Value skeleton = root["skeleton"];
			if (!skeleton.isNull())
			{
				total_bones = skeleton.size();
				
				// allocate enough bones
				mesh->skeleton.allocate(total_bones);
				
				size_t bone_index = 0;
				
				Json::ValueIterator it = skeleton.begin();
				for (; it != skeleton.end(); ++it, ++bone_index)
				{
					const Json::Value& skeleton_entry = (*it);
					const Json::Value& name = skeleton_entry["name"];
					const Json::Value& parent = skeleton_entry["parent"];
					const Json::Value& matrix = skeleton_entry["inverse_bind_pose"];
					
					Joint& joint = mesh->skeleton[bone_index];
					joint.index = bone_index;
					
					if (!name.isNull())
					{
						joint.name = name.asString().c_str();
					}
					
					if (!parent.isNull())
					{
						joint.parent_index = parent.asInt();
					}
					
					if (!matrix.isNull())
					{
						joint.inverse_bind_matrix = assets::json_to_mat4(matrix);
						joint.bind_matrix = glm::inverse(joint.inverse_bind_matrix);
					}
					LOGV("read joint: %s, parent: %i, index: %i\n", joint.name(), joint.parent_index, joint.index);
				}
			}
		

			// iterate over all nodes and count how many there are, of each kind
			Json::ValueIterator node_iter = node_root.begin();
			for (; node_iter != node_root.end(); ++node_iter)
			{
				Json::Value node = (*node_iter);
				count_nodes(node, total_nodes, total_meshes, total_bones);
			}
			
			// allocate nodes
			LOGV("nodes: %i, meshes: %i, bones: %i\n", total_nodes, total_meshes, total_bones);
			mesh->geometry.allocate(total_meshes);
			

			
			MeshLoaderState state(mesh);
			
			// traverse over hierarchy
			node_iter = node_root.begin();
			for (; node_iter != node_root.end(); ++node_iter)
			{
				Json::Value node = (*node_iter);
				traverse_nodes(state, node, materials_by_id, is_world);
			}
			
			
			// read animations
#if 0
			Json::Value animations = root["animations"];
			if (!animations.isNull() && total_bones)
			{
				LOGV("model has %i animation(s); reading them...\n", animations.size());
				
				mesh->animation.total_bones = total_bones;
				mesh->animation.transforms.allocate(total_bones);
				
				mesh->animation.scale.allocate(total_bones);
				mesh->animation.rotation.allocate(total_bones);
				mesh->animation.translation.allocate(total_bones);
				
				mesh->animation.track_scale.allocate(total_bones);
				mesh->animation.track_rotation.allocate(total_bones);
				mesh->animation.track_translate.allocate(total_bones);
				
				Json::ValueIterator iter = animations.begin();
				for (; iter != animations.end(); ++iter)
				{
					Json::Value animation = (*iter);
					
					// assume one animation for now.
					mesh->animation.frames_per_second = animation["frames_per_second"].asInt();
					mesh->animation.frame_delay_seconds = (1.0f / (float)mesh->animation.frames_per_second);
					
					LOGV("animation: \"%s\"\n", animation["name"].asString().c_str());

					const Json::Value& nodes_array = animation["nodes"];
					assert(!nodes_array.isNull());
					Json::ValueIterator node_iter = nodes_array.begin();
					size_t node_index = 0;
					for (; node_iter != nodes_array.end(); ++node_iter)
					{
						const Json::Value& jnode = (*node_iter);
	//					StackString<128> node_name = jnode["name"].asString().c_str();
						std::string node_name = jnode["name"].asString().c_str();
//						scenegraph::Node* node = mesh->scene_root->find_child_named(node_name.c_str());
						

						
//						if (!node)
//						{
//							LOGV("ignoring node: %s\n", node_name.c_str());
//							continue;
//						}
//						else
//						{
//							LOGV("found node: %s\n", node_name.c_str());
//						}
	//					assert(node != nullptr);

//						assert(node->has_attributes(scenegraph::ANIMATED));
						
						const Json::Value& scale_keys = jnode["scale"];
						const Json::Value& rotation_keys = jnode["rotation"];
						const Json::Value& translation_keys = jnode["translation"];
						assert(!scale_keys.isNull() && !rotation_keys.isNull() && !translation_keys.isNull());

						// TODO: read into an animation
						
						
						Joint* joint = mesh->find_bone_named(node_name.c_str());
						assert(joint != 0);
						
						LOGV("reading keyframes for bone \"%s\", joint->index = %i\n", joint->name(), joint->index);
						
						
						read_channel(mesh->animation.scale[joint->index], jnode["scale"]);
						read_channel(mesh->animation.rotation[joint->index], jnode["rotation"]);
						read_channel(mesh->animation.translation[joint->index], jnode["translation"]);
						
						read_channel(mesh->animation.track_scale[joint->index], jnode["scale"]);
						read_channel(mesh->animation.track_rotation[joint->index], jnode["rotation"]);
						read_channel(mesh->animation.track_translate[joint->index], jnode["translation"]);
						
//						scenegraph::AnimatedNode* animated_node = static_cast<scenegraph::AnimatedNode*>(node);
//						assert(animated_node != nullptr);
//						if (animated_node)
//						{
//							read_channel(mesh->animation.scale[node_index], jnode["scale"]);
//							read_channel(mesh->animation.rotation[node_index], jnode["rotation"]);
//							read_channel(mesh->animation.translation[node_index], jnode["translation"]);
//
//							animated_node->scale_channel.set_data_source(&mesh->animation.scale[node_index], mesh->animation.frame_delay_seconds);
//							animated_node->rotation_channel.set_data_source(&mesh->animation.rotation[node_index], mesh->animation.frame_delay_seconds);
//							animated_node->translation_channel.set_data_source(&mesh->animation.translation[node_index], mesh->animation.frame_delay_seconds);
//						}
						
						++node_index;
					}
				}
				
			}
#endif
			
			return core::util::ConfigLoad_Success;
		}
		
		
		void mesh_construct_extension( core::StackString<MAX_PATH_SIZE> & extension )
		{
			extension = ".model";
		} // mesh_construct_extension
		
		Geometry::Geometry()
		{
			material_id = 0;
			vertex_count = 0;
			index_count = 0;
			attributes = 0;
			vertexbuffer = 0;
			draw_type = renderer::DRAW_TRIANGLES;
		}
		
		Geometry::~Geometry()
		{
			if ( this->vertexbuffer )
			{
				renderer::driver()->vertexbuffer_destroy( this->vertexbuffer );
			}
		}
		
		void Geometry::render_setup()
		{
			VertexDescriptor descriptor;

			// setup attributes
			if ( attributes == 0 )
			{
				// always has at least a position
				descriptor.add( VD_FLOAT3 );
				
				if ( !normals.empty() )
				{
					ShaderString normals = "normals";
	//				attributes |= find_parameter_mask( normals );
					descriptor.add( VD_FLOAT3 );
				}
				
				if ( !colors.empty() )
				{
					ShaderString colors = "colors";
	//				attributes |= find_parameter_mask( colors );
					descriptor.add( VD_UNSIGNED_BYTE4 );
				}
				
				if ( uvs.size() > 0 )
				{
					ShaderString uv0 = "uv0";
	//				attributes |= find_parameter_mask( uv0 );
					descriptor.add( VD_FLOAT2 );
				}
				
				if ( uvs.size() > 1 )
				{
					ShaderString uv1 = "uv1";
	//				attributes |= find_parameter_mask( uv1 );
					descriptor.add( VD_FLOAT2 );
				}
				
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
				this->vertexbuffer = renderer::driver()->vertexbuffer_from_geometry( descriptor, this );
			}
			
			if ( !this->is_animated() )
			{
				renderer::driver()->vertexbuffer_upload_geometry( this->vertexbuffer, /*descriptor, */ this );
			}
			//		vertexstream.create( this->vertex_count, this->index_count, renderer::BUFFER_STATIC );
			//		this->vertexbuffer = renderer::driver()->vertexbuffer_create( descriptor, this->draw_type, renderer::BUFFER_STATIC, descriptor.calculate_vertex_stride(), this->vertex_count, this->index_count );
		}
		
		
		//AnimationData::Frame::Frame()
	//		rotation{{rotation_value.x}, {rotation_value.y}, {rotation_value.z}, {rotation_value.w}},
	//		translation{{position_value.x}, {position_value.y}, {position_value.z}}
		//{
		
		//}

		Mesh::Mesh()
		{
//			scene_root = 0;
			total_bones = 0;
			is_dirty = true;
		} // Mesh
		
		Mesh::~Mesh()
		{
//			DESTROY(Node, scene_root);
		}

		
	//	void Mesh::alloc( unsigned int num_geometry )
	//	{
	//		total_geometry = num_geometry;
	//		//		geometry = CREATE_ARRAY( Geometry,  num_geometry );
	//	} // alloc
		
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
			
			for( unsigned int geo_id = 0; geo_id < geometry.size(); ++geo_id )
			{
				assets::Geometry * g = &geometry[ geo_id ];
				g->render_setup();
			}
			
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

		AssetLoadStatus mesh_load_callback(const char* path, Mesh* mesh, const AssetParameters& parameters)
		{
			mesh->path = path;
			if (core::util::json_load_with_callback(path, /*mesh_load_from_json*/load_json_model, mesh, true) == core::util::ConfigLoad_Success)
			{
				return AssetLoad_Success;
			}
			
			return AssetLoad_Failure;
		} // mesh_load_callback
		
	} // namespace assets
} // namespace gemini
// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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
#include <gemini/util/configloader.h>
#include <gemini/core/log.h>

#include "assets.h"
#include "assets/asset_mesh.h"
#include "renderer/renderer.h"

#include "skeletalnode.h"

using namespace renderer;


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

	static glm::mat4 json_to_mat4(Json::Value& value)
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
		
		MeshLoaderState(Mesh* input) : current_geometry(0), mesh(input) {}
	};
	
	void traverse_nodes(MeshLoaderState& state, const Json::Value& node, scenegraph::Node* scene_root, MaterialByIdContainer& materials)
	{
		std::string node_type = node["type"].asString();
		scenegraph::Node* node_root = nullptr;
		
		if (node_type == "mesh")
		{
			LOGV("create mesh\n");
			Geometry* geo = &state.mesh->geometry[state.current_geometry++];
			scenegraph::RenderNode* render_node = CREATE(scenegraph::RenderNode);

			node_root = render_node;
			render_node->geometry = geo;
			render_node->type = scenegraph::STATIC_MESH;
						
			Json::Value mesh_root = node["mesh"];
			assert(!mesh_root.isNull());

			Json::Value index_array = mesh_root["indices"];
			Json::Value vertex_array = mesh_root["vertices"];
			Json::Value normal_array = mesh_root["normals"];
			Json::Value uv_sets = mesh_root["uv_sets"];

			// setup materials
			Material * default_material = assets::materials()->get_default();
			geo->material_id = default_material->Id();
			
			Json::Value material_id = mesh_root["material_id"];
			if (!material_id.isNull())
			{
				// assign this material
				auto it = materials.find(material_id.asInt());
				if (it != materials.end())
				{
					const std::string& material_name = it->second;
					Material* material = assets::materials()->load_from_path(material_name.c_str());
					geo->material_id = material->Id();
				}
			}
			render_node->material_id = geo->material_id;
			
			assets::Shader* shader = assets::shaders()->load_from_path("shaders/world");
			render_node->shader_id = shader->Id();
			
			geo->draw_type = renderer::DRAW_INDEXED_TRIANGLES;
			geo->name = node["name"].asString().c_str();
			LOGV("load geometry: %s\n", geo->name());

			// TODO: remove this legacy code
			geo->index_count = index_array.size();
			geo->vertex_count = vertex_array.size();
			LOGV("index_count = %i, vertex_count = %i\n", geo->index_count, geo->vertex_count);
			geo->indices.allocate(index_array.size());
			geo->vertices.allocate(vertex_array.size());
			geo->normals.allocate(vertex_array.size());
			
			LOGV("uv sets: %i\n", uv_sets.size());
			
			geo->uvs.allocate(uv_sets.size());
//			geo->uvs.allocate(vertex_array.size());
//			geo->colors.allocate(vertex_array.size());

			for (int i = 0; i < geo->indices.size(); ++i)
			{
				geo->indices[i] = index_array[i].asInt();
			}
			
			for (int v = 0; v < geo->vertices.size(); ++v)
			{
				const Json::Value& vertex = vertex_array[v];
				geo->vertices[v] = glm::vec3(vertex[0].asFloat(), vertex[1].asFloat(), vertex[2].asFloat());
//				LOGV("v: %i | %g %g %g\n", v, geo->vertices[v].x, geo->vertices[v].y, geo->vertices[v].z);
				
				const Json::Value& normal = normal_array[v];
				geo->normals[v] = glm::vec3(normal[0].asFloat(), normal[1].asFloat(), normal[2].asFloat());
			}
			
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
		}
		else if (node_type == "skeleton")
		{
			LOGV("create skeleton\n");
			node_root = CREATE(scenegraph::AnimatedNode);
		}
		else
		{
			LOGV("create group node\n");
			node_root = CREATE(scenegraph::AnimatedNode);
		}
		
		assert(node_root != nullptr);
		
		scene_root->add_child(node_root);
		node_root->name = node["name"].asString().c_str();

		const Json::Value& scaling = node["scaling"];
		const Json::Value& rotation = node["rotation"];
		const Json::Value& translation = node["translation"];
		assert(!scaling.isNull() && !rotation.isNull() && !translation.isNull());


		from_json(node_root->local_scale, scaling);
		from_json(node_root->local_rotation, rotation);
		from_json(node_root->local_position, translation);
//		LOGV("scale: %g %g %g\n", node_root->local_scale.x, node_root->local_scale.y, node_root->local_scale.z);
//		LOGV("rotation: %g %g %g %g\n", node_root->local_rotation.x, node_root->local_rotation.y, node_root->local_rotation.z, node_root->local_rotation.w);
//		LOGV("translation: %g %g %g\n", node_root->local_position.x, node_root->local_position.y, node_root->local_position.z);
	
		const Json::Value& children = node["children"];
		if (!children.isNull())
		{
			Json::ValueIterator child_iter = children.begin();
			for (; child_iter != children.end(); ++child_iter)
			{
				traverse_nodes(state, (*child_iter), node_root, materials);
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
		else if (node_type == "skeleton")
		{
			++total_bones;
		}
		
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
	
	util::ConfigLoadStatus load_json_model(const Json::Value& root, void* data)
	{
		Mesh* mesh = (Mesh*)data;
		
		// make sure we can load the default material
		assets::Material* default_mat = assets::materials()->load_from_path("materials/default");
		if (!default_mat)
		{
			LOGE("Could not load the default material!\n");
			return util::ConfigLoad_Failure;
		}
		
		mesh->scene_root = CREATE(scenegraph::Node);
		mesh->scene_root->name = mesh->path();
		
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
			traverse_nodes(state, node, mesh->scene_root, materials_by_id);
		}
		
		
		// read animations
		Json::Value animations = root["animations"];
		if (!animations.isNull())
		{
			LOGV("model has %i animation(s); reading them...\n", animations.size());
			
			mesh->animation.scale.allocate(total_nodes);
			mesh->animation.rotation.allocate(total_nodes);
			mesh->animation.translation.allocate(total_nodes);
			
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
					scenegraph::Node* node = mesh->scene_root->find_child_named(node_name.c_str());
					

					
					if (!node)
					{
						LOGV("ignoring node: %s\n", node_name.c_str());
						continue;
					}
					else
					{
						LOGV("found node: %s\n", node_name.c_str());
					}
//					assert(node != nullptr);

					assert(node->has_attributes(scenegraph::ANIMATED));
					
					const Json::Value& scale_keys = jnode["scale"];
					const Json::Value& rotation_keys = jnode["rotation"];
					const Json::Value& translation_keys = jnode["translation"];
					assert(!scale_keys.isNull() && !rotation_keys.isNull() && !translation_keys.isNull());
				
					scenegraph::AnimatedNode* animated_node = static_cast<scenegraph::AnimatedNode*>(node);
					assert(animated_node != nullptr);
					if (animated_node)
					{
						read_channel(mesh->animation.scale[node_index], jnode["scale"]);
						read_channel(mesh->animation.rotation[node_index], jnode["rotation"]);
						read_channel(mesh->animation.translation[node_index], jnode["translation"]);

						animated_node->scale_channel.set_data_source(&mesh->animation.scale[node_index], mesh->animation.frame_delay_seconds);
						animated_node->rotation_channel.set_data_source(&mesh->animation.rotation[node_index], mesh->animation.frame_delay_seconds);
						animated_node->translation_channel.set_data_source(&mesh->animation.translation[node_index], mesh->animation.frame_delay_seconds);
					}
					
					++node_index;
				}
			}
			
		}
		
		return util::ConfigLoad_Success;
	}
	
	
	void mesh_construct_extension( StackString<MAX_PATH_SIZE> & extension )
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
	
#if 0
	void Geometry::alloc_vertices( unsigned int num_vertices )
	{
		vertex_count = num_vertices;
		vertices = CREATE_ARRAY( glm::vec3, num_vertices );
	} // alloc_vertices
	
	void Geometry::alloc_indices( unsigned int num_indices )
	{
		index_count = num_indices;
		indices = CREATE_ARRAY( renderer::IndexType, num_indices );
	} // alloc_indices
#endif
	
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
				ShaderString hardware_skinning = "hardware_skinning";
//				attributes |= find_parameter_mask(hardware_skinning);
				
				ShaderString node_transforms = "node_transforms";
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
	} // Mesh
	
	Mesh::~Mesh()
	{
		DESTROY(Node, scene_root);
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
		assert(geometry.size() != 0);
		
		for( unsigned int geo_id = 0; geo_id < geometry.size(); ++geo_id )
		{
			assets::Geometry * g = &geometry[ geo_id ];
			g->render_setup();
		}
	} // prepare_geometry

	AssetLoadStatus mesh_load_callback(const char* path, Mesh* mesh, const AssetParameters& parameters)
	{
		mesh->path = path;
		if (util::json_load_with_callback(path, /*mesh_load_from_json*/load_json_model, mesh, true) == util::ConfigLoad_Success)
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // mesh_load_callback
	
}; // namespace assets

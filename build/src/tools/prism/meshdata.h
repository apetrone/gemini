// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#pragma once

#include <map>
#include <string>

#include <gemini/mem.h>
#include <gemini/core.h>
#include <gemini/core/filesystem.h>
#include <gemini/util/fixedarray.h>
#include <gemini/mathlib.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <json/json.h>


namespace prism
{
	const int MAX_BONES = 60;
	const int MAX_VERTEX_WEIGHTS = 4;
	
	struct ToolEnvironment;

	typedef int NodeIndex;
	typedef std::vector< struct Node*, GeminiAllocator<struct Node*> > NodeVector;
	
	typedef std::map<std::string, unsigned int, std::less<std::string>, GeminiAllocator<std::string> > MaterialMap;
	
	
	struct Node
	{
		enum NodeType
		{
			SCENEROOT,
			TRANSFORM,
			BONE,
			MESH
		};
	
		// for bones
		aiMatrix4x4 inverse_bind_pose;
		
		aiMatrix4x4 local_transform;
		
		aiMatrix4x4 world_transform;
		
		// node's name
		std::string name;
		
		NodeType type;
		int32_t bone_index;

		NodeIndex index;
		Node* parent;
		NodeVector children;
		
		Node();
		~Node();
		
		void add_child(Node* node);
		void remove_child(Node* node);
		Node* find_child_with_name(const std::string& name);
		void print();
		
		inline bool is_bone() const { return type == BONE; }
	};


	struct Animation
	{
		struct NodeData
		{
			std::string name;
			
			// TODO: can also support translation and scaling
			// but for now we're just going to support rotation
			glm::quat rotation;
		};
		
		FixedArray<NodeData> nodes;
	};
	
	struct SceneInfo
	{
		MaterialMap& material_map;
		aiMatrix4x4 accumulated_transform;
		ToolEnvironment& env;
		const aiScene* scene;
		Json::Value& geometry_array;
		Json::Value& bones_array;
		
		
		SceneInfo(
			ToolEnvironment& toolenv,
			MaterialMap& mmap,
			const aiScene* aiscene,
			Json::Value& geometry,
			Json::Value& bones
			) :
			material_map(mmap),
			env(toolenv),
			scene(aiscene),
			geometry_array(geometry),
			bones_array(bones)
		{
			
		}
	};
	
	
	// TODO: rename to SceneData
	struct MeshData
	{
		Node* root;
		
		// bone ids are contiguous; keep track
		size_t next_bone_id;
		
		size_t written_meshes;
		size_t ignored_meshes;
		
		MeshData();
		~MeshData();
				

		Node* create_node(const std::string& name, Node::NodeType type, Node* parent = 0);
		Node* find_node_with_name(const std::string& name);
		void print_nodes();
		
		// mesh operations
		void read_bones(ToolEnvironment& env, const aiMesh* mesh, Json::Value& bones, Json::Value& blend_weights);
		
		void read_animation(ToolEnvironment& env, Animation& animation_data, const aiAnimation* animation, Json::Value& animation_node);
		
		void read_mesh(SceneInfo& info, const aiMesh* mesh, Node* node);
	}; // MeshData
	
	struct VertexWeight
	{
		float weight;
		int32_t bone_index;
		
		VertexWeight(float _weight = 0.0f, int32_t _bone_index = -1) :
		weight(_weight), bone_index(_bone_index) {}
	};

	glm::quat to_glm(const aiQuaternion& q);
	glm::vec3 to_glm(const aiVector3D& v);

	void traverse_nodes(SceneInfo& info, MeshData& meshdata, const aiScene* scene);
	void jsonify_quatkey(ToolEnvironment& env, Json::Value& times, Json::Value& values, const aiQuatKey& q);
	void jsonify_vectorkey(ToolEnvironment& env, Json::Value& times, Json::Value& values, const aiVectorKey& v);
	void jsonify_matrix(Json::Value& array, const aiMatrix4x4& source);
	
	void read_vector_keys(ToolEnvironment& env, Json::Value& keys, aiVectorKey* vectorkeys, size_t total_keys);
	void read_quat_keys(ToolEnvironment& env, Json::Value& keys, aiQuatKey* quatkeys, size_t total_keys);
	
	bool validate_frames_per_second(float& frames_per_second);
}; // namespace prism

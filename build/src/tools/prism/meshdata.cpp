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
#include <stdio.h>
#include <stdlib.h>

#include <gemini/mem.h>
#include <gemini/core/log.h>
#include <slim/xlog.h>

#include <gemini/util/stackstring.h>

#include "meshdata.h"

namespace prism
{
	Node::Node()
	{
		type = TRANSFORM;
		index = -1;
		parent = 0;
		bone_index = -1;
	}
	
	Node::~Node()
	{
		NodeVector::iterator it = children.begin();
		for( ; it != children.end(); ++it)
		{
			Node* node = (*it);
			DESTROY(Node, node);
		}

		children.clear();
	}

	void Node::add_child(Node *node)
	{
		if (node->parent)
		{
			node->parent->remove_child(node);
		}
		//LOGV("add child \"%s\" as parent of \"%s\"\n", node->name.c_str(), name.c_str());
		node->parent = this;
		children.push_back(node);
	}
	
	void Node::remove_child(Node* node)
	{
		// find the child and detach from the old parent
		for (NodeVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			if (node == (*it))
			{
				children.erase(it);
				break;
			}
		}
	}
	
	Node* Node::find_child_with_name(const std::string &name)
	{
		if (this->name == name)
		{
			return this;
		}
		
		for (NodeVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			Node* child = (*it)->find_child_with_name(name);
			if (child)
			{
				return child;
			}
		}

		return 0;
	}
	
	void Node::print()
	{
		StackString<64> type_string = "SCENEROOT";
		if (type == Node::TRANSFORM)
		{
			type_string = "TRANSFORM";
		}
		else if (type == Node::BONE)
		{
			type_string = "BONE";
		}
		else if (type == Node::MESH)
		{
			type_string = "MESH";
		}
		LOGV("[Node %x, name=\"%s\", type=\"%s\"\n", this, this->name.c_str(), type_string());
		for (NodeVector::iterator it = children.begin(); it != children.end(); ++it)
		{
			(*it)->print();
		}
	}

	MeshData::MeshData()
	{
		next_bone_id = 0;
		root = CREATE(Node);
		root->name = "scene_root";
		root->type = Node::SCENEROOT;
	}

	MeshData::~MeshData()
	{
		DESTROY(Node, root);
	}
	
	Node* MeshData::create_node(const std::string &name, 
Node::NodeType type, Node* parent)
	{
		Node* node = CREATE(Node);
		node->name = name;
		node->type = type;

		// add this as a child to the parent (or root node)
		if (!parent)
		{
			parent = root;
		}
		parent->add_child(node);
		
		return node;
	}
	
	Node* MeshData::find_node_with_name(const std::string& name)
	{
		Node* node = root->find_child_with_name(name);
		if (!node)
		{
			LOGV("unable to find node named \"%s\"\n", name.c_str());
		}
		
		return node;
	}
	
	void MeshData::print_nodes()
	{
		if (root)
		{
			root->print();
		}
	}
	
	void MeshData::read_bones(const aiMesh* mesh, Json::Value& bones)
	{
		if (mesh->mNumBones > 0)
		{
			LOGV("inspecting bones...\n");
			size_t total_bones = 0;
			const aiBone* bone = 0;
			for (size_t boneid = 0; boneid < mesh->mNumBones; ++boneid)
			{
				Json::Value jbone;
				bone = mesh->mBones[boneid];
				LOGV("\tbone %i, \"%s\"\n", boneid, bone->mName.C_Str());
				LOGV("\tweights: %i\n", bone->mNumWeights);
				
				
				
				jbone["name"] = bone->mName.C_Str();
				//jbone["bone_index"] = Json::valueToString((unsigned)total_bones);
				
				
				aiMatrix4x4 offset = bone->mOffsetMatrix;
				Json::Value offset_matrix(Json::arrayValue);
				jsonify_matrix(offset_matrix, bone->mOffsetMatrix);
				jbone["inverse_bind_pose"] = offset_matrix;
							
				Json::Value jtransform;
										
				Node* node = find_node_with_name(bone->mName.C_Str());
				if (node)
				{
					node->type = Node::BONE;
					node->bone_index = total_bones;
					jsonify_matrix(jtransform, node->local_transform);
				}
				
				jbone["transform"] = jtransform;
				
				total_bones++;
				
				Json::Value weights(Json::arrayValue);
				for (size_t weight = 0; weight < bone->mNumWeights; ++weight)
				{
					aiVertexWeight* w = &bone->mWeights[weight];
//					LOGV("\tweight (%i) [vertex: %i -> weight: %2.2f\n", weight, w->mVertexId, w->mWeight);
					
					Json::Value jweight;
					jweight["id"] = Json::valueToString((unsigned int)w->mVertexId);
					jweight["weight"] = Json::valueToString(w->mWeight);
					weights.append(jweight);
				}
				
				jbone["weights"] = weights;
			
				bones.append(jbone);
			}
		}
	}

	void MeshData::read_animation(Animation& animation_data, const aiAnimation* animation, Json::Value& animation_node)
	{

		LOGV("\tduration: %g\n", animation->mDuration);
		LOGV("\tticks_per_second: %g\n", animation->mTicksPerSecond);
		LOGV("\tbone channels (skeletal): %i\n", animation->mNumChannels);
		LOGV("\tmesh channels (vertex): %i\n", animation->mNumMeshChannels);
		
		animation_node["name"] = animation->mName.C_Str();
		animation_node["duration_seconds"] = animation->mDuration;
		float ticks_per_second = animation->mTicksPerSecond;
		validate_frames_per_second(ticks_per_second);
		animation_node["ticks_per_second"] = ticks_per_second;
		
		Json::Value node_list;
		
		Animation test;
		
		
		// bone/node-based animation
		const aiNodeAnim* animnode = 0;
		for (size_t channel = 0; channel < animation->mNumChannels; ++channel)
		{
			Json::Value node;
			animnode = animation->mChannels[channel];
			
			LOGV("\tinspecting bone/node %llu \"%s\" ...\n", channel, animnode->mNodeName.C_Str());
			LOGV("\t\t Total Keys: %i\n", animnode->mNumPositionKeys);
			assert((animnode->mNumPositionKeys == animnode->mNumRotationKeys) && (animnode->mNumRotationKeys == animnode->mNumScalingKeys));
			
			
			Json::Value jkeys;
			
			std::string node_name = animnode->mNodeName.C_Str();
			
//			node["name"] = animnode->mNodeName.C_Str();
			node["name"] = node_name;
			
			int32_t bone_index = -1;
			int32_t parent_bone_index = -1;
			
			Node* bone_node = find_node_with_name(node_name);
			if (bone_node)
			{
				bone_index = bone_node->bone_index;
				if (bone_node->parent)
				{
					parent_bone_index = bone_node->parent->bone_index;
				}
			}
			
			node["bone_index"] = bone_index;
			node["bone_parent"] = parent_bone_index;
			
			Json::Value scale_keys;
			Json::Value rotation_keys;
			Json::Value position_keys;

			read_vector_keys(scale_keys, animnode->mScalingKeys, animnode->mNumScalingKeys);
			read_quat_keys(rotation_keys, animnode->mRotationKeys, animnode->mNumRotationKeys);
			read_vector_keys(position_keys, animnode->mPositionKeys, animnode->mNumPositionKeys);
			
			jkeys["scale"] = scale_keys;
			jkeys["rotation"] = rotation_keys;
			jkeys["translation"] = position_keys;
			
			node["keys"] = jkeys;
			node_list.append(node);
		
			animation_node["nodes"] = node_list;
		}

//		// vertex-based animation
//		const aiMeshAnim* anim = 0;
//		for (size_t channel = 0; channel < animation->mNumMeshChannels; ++channel)
//		{
//			anim = animation->mMeshChannels[channel];
//		}
		
		
	}

	glm::quat to_glm(const aiQuaternion& q)
	{
		return glm::quat(q.x, q.y, q.z, q.w);
	}

	glm::vec3 to_glm(const aiVector3D& v)
	{
		return glm::vec3(v.x, v.y, v.z);
	}
	
	void jsonify_quatkey(Json::Value& times, Json::Value& values, const aiQuatKey& qkey)
	{
		const aiQuaternion& q = qkey.mValue;
		times.append(qkey.mTime);
		values.append(q.x);
		values.append(q.y);
		values.append(q.z);
		values.append(q.w);
	}

	void jsonify_vectorkey(Json::Value& times, Json::Value& values, const aiVectorKey& vkey)
	{
		const aiVector3D& v = vkey.mValue;
		times.append(vkey.mTime);
		values.append(v.x);
		values.append(v.y);
		values.append(v.z);
	}

	void jsonify_matrix(Json::Value& array, const aiMatrix4x4& source)
	{
		// The matrix must be transposed before copying to json
		// because aiMatrix4x4 is ALWAYS row-major.
		aiMatrix4x4 matrix = source;
		matrix.Transpose();
		
		array.append(matrix.a1);
		array.append(matrix.a2);
		array.append(matrix.a3);
		array.append(matrix.a4);
		
		array.append(matrix.b1);
		array.append(matrix.b2);
		array.append(matrix.b3);
		array.append(matrix.b4);
		
		array.append(matrix.c1);
		array.append(matrix.c2);
		array.append(matrix.c3);
		array.append(matrix.c4);
		
		array.append(matrix.d1);
		array.append(matrix.d2);
		array.append(matrix.d3);
		array.append(matrix.d4);
	}

	void read_vector_keys(Json::Value& keys, aiVectorKey* vectorkeys, size_t total_keys)
	{
		Json::Value times;
		Json::Value values;
		
		for (size_t i = 0; i < total_keys; ++i)
		{
			jsonify_vectorkey(times, values, vectorkeys[i]);
		}
		
		keys["time"] = times;
		keys["value"] = values;
	}
	
	void read_quat_keys(Json::Value& keys, aiQuatKey* quatkeys, size_t total_keys)
	{
		Json::Value times;
		Json::Value values;
		
		for (size_t i = 0; i < total_keys; ++i)
		{
			jsonify_quatkey(times, values, quatkeys[i]);
		}
		
		keys["time"] = times;
		keys["value"] = values;
	}

	// this function is almost identical to the one in the assimp documentation
	void iterate_nodes(MeshData& meshdata, aiNode* node, Node* parent, aiMatrix4x4& accumulated_transform, size_t& total_nodes)
	{
		//LOGV("[node %i] %s\n", total_nodes, node->mName.C_Str());
		++total_nodes;
				
		Node* newnode = meshdata.create_node(node->mName.C_Str(), Node::TRANSFORM, parent);
		//LOGV("created node %x, %s\n", newnode, newnode->name.c_str());
		
		// if node has meshes, create a new scene object for it
		if (node->mNumMeshes > 0)
		{
			//LOGV("\tnode has %i meshes\n", node->mNumMeshes);
			newnode->type = Node::MESH;
		}
		else
		{
			// if no meshes, skip the node, but keep its transform
			newnode->local_transform = node->mTransformation * accumulated_transform;
		}
				
		// traverse all child nodes
		//LOGV("[node] %s has %i children.\n", node->mName.C_Str(), node->mNumChildren);
		for(size_t child_index = 0; child_index < node->mNumChildren; ++child_index)
		{
			iterate_nodes(meshdata, node->mChildren[child_index], newnode, accumulated_transform, total_nodes);
		}
	}

	void traverse_nodes(MeshData& meshdata, const aiScene* scene, Json::Value& hierarchy)
	{
		LOGV("iterating over scene nodes...\n");
		aiMatrix4x4 accumulated_transform;
		size_t total_nodes = 0;
		iterate_nodes(meshdata, scene->mRootNode, meshdata.root, accumulated_transform, total_nodes);
		
		LOGV("scene nodes traversed.\n");
	}
	
	bool validate_frames_per_second(float& frames_per_second)
	{
		if (frames_per_second < 2)
		{
			LOGW("frames per second is below the minimum of: 30\n");
			LOGW("corrected.\n");
			frames_per_second = 30;
			return true;
		}
		
		return false;
	} // move this?
}; // namespace prism

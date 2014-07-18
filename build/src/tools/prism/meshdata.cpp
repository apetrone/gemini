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
		type = NodeType::TRANSFORM;
		index = -1;
		parent = 0;
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
		LOGV("add child \"%s\" as parent of \"%s\"\n", node->name.c_str(), name.c_str());
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
		if (type == NodeType::TRANSFORM)
		{
			type_string = "TRANSFORM";
		}
		else if (type == NodeType::BONE)
		{
			type_string = "BONE";
		}
		else if (type == NodeType::MESH)
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
		root->type = NodeType::SCENEROOT;
	}

	MeshData::~MeshData()
	{
		DESTROY(Node, root);
	}
	
	Node* MeshData::create_node(const std::string &name, NodeType type, Node* parent)
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
			const aiBone* bone = 0;
			for (size_t boneid = 0; boneid < mesh->mNumBones; ++boneid)
			{
				Json::Value jbone;
				bone = mesh->mBones[boneid];
				LOGV("\tbone %i, \"%s\"\n", boneid, bone->mName.C_Str());
				LOGV("\tweights: %i\n", bone->mNumWeights);
				
				
				
				jbone["name"] = bone->mName.C_Str();
				
				aiMatrix4x4 offset = bone->mOffsetMatrix;
				Json::Value offset_matrix(Json::arrayValue);
				jsonify_matrix(offset_matrix, bone->mOffsetMatrix);
				jbone["inverse_bind_pose"] = offset_matrix;
				
				Node* node = find_node_with_name(bone->mName.C_Str());
				if (node)
				{
					node->type = NodeType::BONE;
				}
				
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

	glm::quat to_glm(const aiQuaternion& q)
	{
		return glm::quat(q.x, q.y, q.z, q.w);
	}

	glm::vec3 to_glm(const aiVector3D& v)
	{
		return glm::vec3(v.x, v.y, v.z);
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


	// this function is almost identical to the one in the assimp documentation
	void iterate_nodes(MeshData& meshdata, aiNode* node, Node* parent, aiMatrix4x4& accumulated_transform, size_t& total_nodes)
	{
		LOGV("[node %i] %s\n", total_nodes, node->mName.C_Str());
		++total_nodes;
				
		Node* newnode = meshdata.create_node(node->mName.C_Str(), NodeType::TRANSFORM, parent);
		LOGV("created node %x, %s\n", newnode, newnode->name.c_str());
		
		// if node has meshes, create a new scene object for it
		if (node->mNumMeshes > 0)
		{
			LOGV("\tnode has %i meshes\n", node->mNumMeshes);
			newnode->type = NodeType::MESH;
		}
		else
		{
			// if no meshes, skip the node, but keep its transform
			node->mTransformation * accumulated_transform;
		}
		
		// traverse all child nodes
		LOGV("[node] %s has %i children.\n", node->mName.C_Str(), node->mNumChildren);
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
}; // namespace prism

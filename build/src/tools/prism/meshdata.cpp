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
#include "prism.h"

#include <gemini/mathlib.h>

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
//		LOGV("[Node %x, name=\"%s\", type=\"%s\"\n", this, this->name.c_str(), type_string());
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
	
	Node* MeshData::create_node(const std::string &name, Node::NodeType type, Node* parent)
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
	
	void MeshData::read_bones(ToolEnvironment& env, const aiMesh* mesh, Json::Value& bones, Json::Value& out_blend_weights)
	{
		std::vector< std::vector<VertexWeight> > indices;
		indices.resize(mesh->mNumVertices);
		
		if (mesh->mNumBones > 0)
		{
			if (mesh->mNumBones > MAX_BONES)
			{
				LOGW("Maximum number of supported bones has been exceeded!\n");
				assert(0);
			}
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
				
				aiMatrix4x4 coordinate_transform(env.coordinate_transform);
				aiMatrix4x4 offset = bone->mOffsetMatrix * coordinate_transform;
				Json::Value offset_matrix(Json::arrayValue);
				jsonify_matrix(offset_matrix, offset);
				jbone["inverse_bind_pose"] = offset_matrix;
				
				
				Json::Value jtransform;
				Node* node = find_node_with_name(bone->mName.C_Str());
				if (node)
				{
					node->type = Node::BONE;
					node->bone_index = total_bones;
					jsonify_matrix(jtransform, node->local_transform);
				}
				
				// node transforms are not used in the case of bones.
#if 0
				jbone["transform"] = jtransform;
#endif
				total_bones++;
				
				Json::Value weights(Json::arrayValue);
				for (size_t weight = 0; weight < bone->mNumWeights; ++weight)
				{
					aiVertexWeight* w = &bone->mWeights[weight];
					//LOGV("\tweight (%i) [vertex: %i -> weight: %2.2f\n", weight, w->mVertexId, w->mWeight);
					
					indices[w->mVertexId].push_back(VertexWeight(w->mWeight, boneid));
					if (indices[w->mVertexId].size() >= MAX_VERTEX_WEIGHTS)
					{
						LOGW("Exceeded maximum vertex weight limit of (%i). This may produce undesirable results.\n", MAX_VERTEX_WEIGHTS);
					}
				}

				bones.append(jbone);
			}
			
			// blend weights will look like:
//			"blend_weights": [
//				// implicit index of 0
//				{
//					"indices": [0, 1],
//					"weights:" [0.0, 1.0]
//				}
//			]
			
			for (size_t v = 0; v < mesh->mNumVertices; ++v)
			{
				Json::Value jweight;
				Json::Value blend_indices(Json::arrayValue);
				Json::Value blend_weights(Json::arrayValue);
				for (size_t i = 0; i < indices[v].size(); ++i)
				{
					const VertexWeight& vw = indices[v][i];
					blend_indices.append(vw.bone_index);
					blend_weights.append(vw.weight);
				}
				
				jweight["indices"] = blend_indices;
				jweight["weights"] = blend_weights;
				
				out_blend_weights.append(jweight);
			}
		}
	}

	void MeshData::read_animation(ToolEnvironment& env, Animation& animation_data, const aiAnimation* animation, Json::Value& animation_node)
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
			
//			LOGV("\tinspecting bone/node %llu \"%s\" ...\n", channel, animnode->mNodeName.C_Str());
//			LOGV("\t\t Total Keys: %i\n", animnode->mNumPositionKeys);
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

			read_vector_keys(env, scale_keys, animnode->mScalingKeys, animnode->mNumScalingKeys);
			read_quat_keys(env, rotation_keys, animnode->mRotationKeys, animnode->mNumRotationKeys);
			read_vector_keys(env, position_keys, animnode->mPositionKeys, animnode->mNumPositionKeys);
			
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
	
	void MeshData::read_mesh(SceneInfo& info, const aiMesh* mesh, Node* node)
	{
		assert(mesh != 0);
		assert(node != 0);
		
		Json::Value jgeometry;
		
		// per-vertex attributes
		Json::Value jvertices(Json::arrayValue);
		Json::Value jnormals(Json::arrayValue);
		Json::Value juvs(Json::arrayValue);
		Json::Value jcolors(Json::arrayValue);
		Json::Value jblend_weights(Json::arrayValue);
		
		
		Json::Value jfaces(Json::arrayValue);
		
		
		// any one of these is an error condition otherwise.
		//		assert( mesh->HasTextureCoords(0) );
		assert( mesh->HasNormals() );
		//		assert( mesh->HasTangentsAndBitangents() );
		
		jgeometry["name"] = mesh->mName.C_Str();
		LOGV("read mesh: \"%s\"\n", mesh->mName.C_Str());
//		LOGV("inspecting mesh: %i, \"%s\"\n", m, mesh->mName.C_Str());
		//		LOGV("\tvertices: %i\n", mesh->mNumVertices);
		//		LOGV("\tfaces: %i\n", mesh->mNumFaces);
		//		LOGV("\tbones: %i\n", mesh->mNumBones);
		
		read_bones(info.env, mesh, info.bones_array, jblend_weights);

		jgeometry["blend_weights"] = jblend_weights;
		
		if (mesh->HasNormals())
		{
			for(unsigned int vertex = 0; vertex < mesh->mNumVertices; ++vertex)
			{
				const aiVector3D & pos = mesh->mVertices[vertex];
				const aiVector3D & normal = mesh->mNormals[vertex];
				
				
				//const aiVector3D & bitangent = mesh->mBitangents[vertex];
				//const aiVector3D & tangent = mesh->mTangents[vertex];
				
				const aiVector3D tr_pos = info.env.coordinate_transform * node->local_transform * pos;
				
				jvertices.append(tr_pos.x);
				jvertices.append(tr_pos.y);
				jvertices.append(tr_pos.z);
				
				const aiVector3D tr_normal = info.env.coordinate_transform * node->local_transform * normal;
				
				jnormals.append(tr_normal.x);
				jnormals.append(tr_normal.y);
				jnormals.append(tr_normal.z);
				
				if (mesh->HasTextureCoords(0))
				{
					const aiVector3D & uv = mesh->mTextureCoords[0][vertex];
					juvs.append(uv.x);
					juvs.append(uv.y);
				}
				
				if (mesh->HasVertexColors(0))
				{
					const aiColor4D& color = mesh->mColors[0][vertex];
					jcolors.append(color.r);
					jcolors.append(color.g);
					jcolors.append(color.b);
					jcolors.append(color.a);
				}
			}
			
			aiFace* face;
			for (uint32_t face_index = 0; face_index < mesh->mNumFaces; ++face_index)
			{
				face = &mesh->mFaces[face_index];
				jfaces.append(face->mIndices[0]);
				jfaces.append(face->mIndices[1]);
				jfaces.append(face->mIndices[2]);
			}
			
			jgeometry["positions"] = jvertices;
			jgeometry["normals"] = jnormals;
			jgeometry["uvs"] = juvs;
			jgeometry["colors"] = jcolors;
			jgeometry["indices"] = jfaces;
		}
		else
		{
//			fprintf(stdout, "Mesh %zu is missing Normals\n", m);
			LOGW("Mesh \"%s\" is missing normals.\n", mesh->mName.C_Str());
		}
		
		
		//		LOGV("material index: %u\n", mesh->mMaterialIndex);
		
		// TODO: error checking here...
		aiMaterial* material = info.scene->mMaterials[mesh->mMaterialIndex];
		
		//		if (material->GetTextureCount(aiTextureType_AMBIENT) > 0) { LOGV("material has an ambient texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) { LOGV("material has a diffuse texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_DISPLACEMENT) > 0) { LOGV("material has a displacement texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0) { LOGV("material has an emissive texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_HEIGHT) > 0) { LOGV("material has a heightmap texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_LIGHTMAP) > 0) { LOGV("material has a lightmap texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_NONE) > 0) { LOGV("material has a 'None' texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_NORMALS) > 0) { LOGV("material has a normals texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_OPACITY) > 0) { LOGV("material has an opacity texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_REFLECTION) > 0) { LOGV("material has a reflection texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_SHININESS) > 0) { LOGV("material has a shininess texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) { LOGV("material has a specular texture\n"); }
		//		if (material->GetTextureCount(aiTextureType_UNKNOWN) > 0) { LOGV("material has an unknown texture\n"); }
		
		//		LOGV("material has %i diffuse texture(s)\n", material->GetTextureCount(aiTextureType_DIFFUSE));
		
		aiString material_name;
		material->Get(AI_MATKEY_NAME, material_name);
		
		//		LOGV("mesh = \"%s\", material name: \"%s\", index: %u\n", mesh->mName.C_Str(), material_name.C_Str(), mesh->mMaterialIndex);
		
		aiString texture_path;
		if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path, 0, 0, 0, 0, 0))
		{
			StackString<4096> texpath = texture_path.C_Str();
			
			// TODO: verify there's a material by this name in the input/output folder?
			
			// COLLADA does some weird shit with texture names.
//			jgeometry["material_id"] = jmaterial_array.size();
			Json::Value jmaterial;
			std::string material_name = texpath.basename().remove_extension()();
			
			MaterialMap::iterator it = info.material_map.find(material_name);
			if (it != info.material_map.end())
			{
				// already exists in the material map
				jgeometry["material_id"] = (*it).second;
			}
			else
			{
				unsigned int next_material_id = (unsigned int)info.material_map.size();
				jgeometry["material_id"] = next_material_id;
				info.material_map.insert(MaterialMap::value_type(material_name, next_material_id));
			}
			
			//			LOGV("diffuse texture: %s\n", material_name.c_str());
		}
		else
		{
			LOGW("mesh \"%s\" has no material!\n", mesh->mName.C_Str());
		}
		
		info.geometry_array.append(jgeometry);
	}
	
	glm::quat to_glm(const aiQuaternion& q)
	{
		return glm::quat(q.x, q.y, q.z, q.w);
	}

	glm::vec3 to_glm(const aiVector3D& v)
	{
		return glm::vec3(v.x, v.y, v.z);
	}
	
	void jsonify_quatkey(ToolEnvironment& env, Json::Value& times, Json::Value& values, const aiQuatKey& qkey)
	{
		aiQuaternion q = qkey.mValue;
		
		if (env.convert_zup_to_yup)
		{
			q = aiQuaternion(aiVector3D(1, 0, 0), -1.57079633f) * qkey.mValue;
		}
		
		times.append(qkey.mTime);
		values.append(q.x);
		values.append(q.y);
		values.append(q.z);
		values.append(q.w);
	}

	void jsonify_vectorkey(ToolEnvironment& env, Json::Value& times, Json::Value& values, const aiVectorKey& vkey)
	{
		aiVector3D v = vkey.mValue;
		
		if (env.convert_zup_to_yup)
		{
			v = env.coordinate_transform * vkey.mValue;
		}
		
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

	void read_vector_keys(ToolEnvironment& env, Json::Value& keys, aiVectorKey* vectorkeys, size_t total_keys)
	{
		Json::Value times;
		Json::Value values;
		
		for (size_t i = 0; i < total_keys; ++i)
		{
			jsonify_vectorkey(env, times, values, vectorkeys[i]);
		}
		
		keys["time"] = times;
		keys["value"] = values;
	}
	
	void read_quat_keys(ToolEnvironment& env, Json::Value& keys, aiQuatKey* quatkeys, size_t total_keys)
	{
		Json::Value times;
		Json::Value values;
		
		for (size_t i = 0; i < total_keys; ++i)
		{
			jsonify_quatkey(env, times, values, quatkeys[i]);
		}
		
		keys["time"] = times;
		keys["value"] = values;
	}

	// this function is almost identical to the one in the assimp documentation
	void iterate_nodes(
		SceneInfo& info,
		MeshData& meshdata,
		aiNode* node,
		Node* parent,
		aiMatrix4x4& accumulated_transform,
		size_t& total_nodes
		)
	{
//		LOGV("[node %i] %s\n", total_nodes, node->mName.C_Str());
		++total_nodes;
				
		Node* newnode = meshdata.create_node(node->mName.C_Str(), Node::TRANSFORM, parent);
		LOGV("created node \"%s\"\n", newnode->name.c_str());
		
		newnode->local_transform = node->mTransformation * accumulated_transform;
		
		// if node has meshes, create a new scene object for it
		if (node->mNumMeshes > 0)
		{
			LOGV("\t# submeshes: %i\n", node->mNumMeshes);
			
			newnode->type = Node::MESH;
			for (unsigned int mid = 0; mid < node->mNumMeshes; ++mid)
			{
				aiMesh* submesh = info.scene->mMeshes[node->mMeshes[mid]];
				meshdata.read_mesh(info, submesh, newnode);
			}
		}
				
		// traverse all child nodes
		//LOGV("[node] %s has %i children.\n", node->mName.C_Str(), node->mNumChildren);
		for(size_t child_index = 0; child_index < node->mNumChildren; ++child_index)
		{
			iterate_nodes(info, meshdata, node->mChildren[child_index], newnode, accumulated_transform, total_nodes);
		}
	}

	void traverse_nodes(SceneInfo& info, MeshData& meshdata, const aiScene* scene, Json::Value& hierarchy)
	{
		LOGV("iterating over scene nodes...\n");
		aiMatrix4x4 accumulated_transform;
		size_t total_nodes = 0;
		iterate_nodes(info, meshdata, scene->mRootNode, meshdata.root, accumulated_transform, total_nodes);

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

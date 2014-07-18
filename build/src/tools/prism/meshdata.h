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

#pragma once

#include <gemini/mem.h>
#include <gemini/core.h>
#include <gemini/core/filesystem.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <json/json.h>

// TODO: replace with mathlib.h at some point?
// show output at build time regarding glm
//#define GLM_MESSAGES 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
//#include <glm/gtc/noise.hpp>
//#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace prism
{

	struct Node
	{
		enum Type
		{
				
		};
		
		aiMatrix4x4 inverse_bind_pose;
		std::string name;
	};

	typedef std::vector<Node, GeminiAllocator<Node>> NodeVector;

	struct MeshData
	{
		NodeVector nodes;
		
		Node* find_node_with_name(const std::string& name);
	}; // MeshData


	glm::quat to_glm(const aiQuaternion& q);
	glm::vec3 to_glm(const aiVector3D& v);

	void iterate_nodes(aiNode* node, aiMatrix4x4& accumulated_transform, size_t& total_nodes);
	void traverse_nodes(const aiScene* scene, Json::Value& hierarchy);
	void jsonify_matrix(Json::Value& array, const aiMatrix4x4& source);
}; // namespace prism
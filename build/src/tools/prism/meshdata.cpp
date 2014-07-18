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

#include "meshdata.h"

namespace prism
{
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
	void iterate_nodes(aiNode* node, aiMatrix4x4& accumulated_transform, size_t& total_nodes)
	{
		
		LOGV("[node %i] %s\n", total_nodes, node->mName.C_Str());
		++total_nodes;
		
		// if node has meshes, create a new scene object for it
		if (node->mNumMeshes > 0)
		{
			LOGV("\tnode has %i meshes\n", node->mNumMeshes);
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
			iterate_nodes(node->mChildren[child_index], accumulated_transform, total_nodes);
		}
	}

	void traverse_nodes(const aiScene* scene, Json::Value& hierarchy)
	{
		LOGV("iterating over scene nodes...\n");
		aiMatrix4x4 accumulated_transform;
		size_t total_nodes = 0;
		iterate_nodes(scene->mRootNode, accumulated_transform, total_nodes);
		
		LOGV("scene nodes traversed.\n");
	}
}; // namespace prism

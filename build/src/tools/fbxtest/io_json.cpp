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
#include "io_json.h"

#include "datamodel/mesh.h"

void JsonSceneWriter::jsonify_matrix(Json::Value& array, glm::mat4& matrix)
{
	array.append(matrix[0].x);
	array.append(matrix[1].x);
	array.append(matrix[2].x);
	array.append(matrix[3].x);
	
	array.append(matrix[0].y);
	array.append(matrix[1].y);
	array.append(matrix[2].y);
	array.append(matrix[3].y);
	
	array.append(matrix[0].z);
	array.append(matrix[1].z);
	array.append(matrix[2].z);
	array.append(matrix[3].z);
	
	array.append(matrix[0].w);
	array.append(matrix[1].w);
	array.append(matrix[2].w);
	array.append(matrix[3].w);
}

void JsonSceneWriter::append_node(datamodel::SceneNode* node, Json::Value& jnodes)
{
	Json::Value jnode;
	jnode["name"] = node->name;
	jnode["type"] = node->type;
	
	Json::Value jscale;
	jscale.append(node->scale.x);
	jscale.append(node->scale.y);
	jscale.append(node->scale.z);
	jnode["scaling"] = jscale;
	
	Json::Value jrotation;
	jrotation.append(node->rotation.x);
	jrotation.append(node->rotation.y);
	jrotation.append(node->rotation.z);
	jrotation.append(node->rotation.w);
	jnode["rotation"] = jrotation;
	
	Json::Value jtranslation;
	jtranslation.append(node->translation.x);
	jtranslation.append(node->translation.y);
	jtranslation.append(node->translation.z);
	jnode["translation"] = jtranslation;
	
	Json::Value child_nodes(Json::arrayValue);
	for (auto child : node->children)
	{
		append_node(child, child_nodes);
	}
	jnode["children"] = child_nodes;
	
	
	if (node->mesh)
	{
		Json::Value mesh_data;
		
		// write vertices
		Json::Value vertices(Json::arrayValue);
		for (size_t vertex_id = 0; vertex_id < node->mesh->vertices.size(); ++vertex_id)
		{
			const glm::vec3& vertex = node->mesh->vertices[vertex_id];
			Json::Value jvertex(Json::arrayValue);
			jvertex.append(vertex.x);
			jvertex.append(vertex.y);
			jvertex.append(vertex.z);
			vertices.append(jvertex);
		}
		mesh_data["vertices"] = vertices;
		
		// write indices
		Json::Value indices(Json::arrayValue);
		for (size_t index_id = 0; index_id < node->mesh->indices.size(); ++index_id)
		{
			indices.append(node->mesh->indices[index_id]);
		}
		mesh_data["indices"] = indices;
		
		// write normals
		Json::Value normals(Json::arrayValue);
		for (size_t normal_id = 0; normal_id < node->mesh->normals.size(); ++normal_id)
		{
			const glm::vec3& normal = node->mesh->normals[normal_id];
			Json::Value jnormal(Json::arrayValue);
			jnormal.append(normal.x);
			jnormal.append(normal.y);
			jnormal.append(normal.z);
			normals.append(jnormal);
		}
		mesh_data["normals"] = normals;
		
		// write vertex_colors
		Json::Value vertex_colors(Json::arrayValue);
		for (size_t color_id = 0; color_id < node->mesh->vertex_colors.size(); ++color_id)
		{
			const glm::vec4& color = node->mesh->vertex_colors[color_id];
			Json::Value jcolor(Json::arrayValue);
			jcolor.append(color.r);
			jcolor.append(color.g);
			jcolor.append(color.b);
			jcolor.append(color.a);
			vertex_colors.append(jcolor);
		}
		mesh_data["vertex_colors"] = vertex_colors;
		
		
		// write uvs
		Json::Value uv_sets(Json::arrayValue);
		for (size_t uv_set_id = 0; uv_set_id < node->mesh->uvs.size(); ++uv_set_id)
		{
			Json::Value juvs(Json::arrayValue);
			for (size_t uv_id = 0; uv_id < node->mesh->uvs[uv_set_id].size(); ++uv_id)
			{
				glm::vec2& uv = node->mesh->uvs[uv_set_id][uv_id];
				Json::Value juv(Json::arrayValue);
				juv.append(uv.x);
				juv.append(uv.y);
				juvs.append(juv);
			}
			uv_sets.append(juvs);
		}
		mesh_data["uv_sets"] = uv_sets;
		
		jnode["mesh"] = mesh_data;
	}
	
	// add this node
	jnodes.append(jnode);
}


void JsonSceneWriter::write(datamodel::SceneNode* root, util::DataStream& source)
{
	Json::Value jroot(Json::arrayValue);
	
	for (auto child : root->children)
	{
		append_node(child, jroot);
	}
	
	Json::StyledWriter writer;
	
	std::string buffer = writer.write(jroot);
	source.write(buffer.data(), buffer.size());
}
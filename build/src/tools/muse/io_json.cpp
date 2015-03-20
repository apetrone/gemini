// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <core/typedefs.h>

#include <core/logging.h>
#include <core/xfile.h>

#include "io_json.h"

#include "datamodel/model.h"
#include "datamodel/mesh.h"
#include "datamodel/material.h"

using namespace core;

using namespace gemini::datamodel;

namespace gemini
{
	typedef glm::quat RotationOutputType;
	
	template <class T>
	bool RotationIsQuaternion() { return false; }
	
	template <>
	bool RotationIsQuaternion<glm::quat>() { return true; }
	
	
	template <class Type>
	void jsonify_value(Json::Value& jvalue, const Type& value);

	template <>
	void jsonify_value(Json::Value& jvalue, const glm::vec3& vector)
	{
		Json::Value v;
		v.append(vector.x);
		v.append(vector.y);
		v.append(vector.z);
		
		jvalue.append(v);
	}

	template <>
	void jsonify_value(Json::Value& jvalue, const glm::quat& quat)
	{
		Json::Value q;
		q.append(quat.x);
		q.append(quat.y);
		q.append(quat.z);
		q.append(quat.w);
		
		jvalue.append(q);
	}
	
	template <class Type, class OutType>
	OutType adapter(const Type& input)
	{
		return input;
	}
	
	template <>
	glm::quat adapter<glm::vec3, glm::quat>(const glm::vec3& input)
	{
		// convert from degrees to radians
		glm::vec3 rads(
			mathlib::degrees_to_radians(input.x),
			mathlib::degrees_to_radians(input.y),
			mathlib::degrees_to_radians(input.z)
		);
		return glm::quat(rads);
	}

	template <class Type, class OutType>
	void gather_keys(Json::Value& jkeys, std::vector<datamodel::Keyframe<OutType>* >& keys)
	{
		Json::Value jtime(Json::arrayValue);
		Json::Value jvalue(Json::arrayValue);
		for (auto key : keys)
		{
			jtime.append(key->time_seconds);
			jsonify_value(jvalue, adapter<Type, OutType>(key->value));
		}
		
		jkeys["time"] = jtime;
		jkeys["value"] = jvalue;
	}

	
	void JsonModelWriter::write_animations(const std::string& abs_base_path, datamodel::Model* model)
	{
		size_t total_animations = model->animations.size();
		if (!total_animations)
		{
			return;
		}

		// this should export a single animation per file.
		
		for (datamodel::Animation* animation : model->animations)
		{
			Json::Value janimation;
			
			janimation["name"] = animation->name.c_str();
//			LOGV("animation: %s\n", animation->name.c_str());
			
			janimation["frames_per_second"] = animation->frames_per_second;

			
			Json::Value jnodes(Json::arrayValue);
			for (NodeAnimation* data : animation->node_animations)
			{
				Json::Value jnode;
				datamodel::Bone* bone = model->skeleton->find_bone_named(data->name);
				
				// animation present for bone that was not added to the skeleton
				assert(bone != 0);
				
				if (bone)
				{
//					LOGV("bone: %s\n", bone->name.c_str());
//					LOGV("# keys: %i %i %i\n", data->translation.keys.size(), data->rotation.keys.size(), data->scale.keys.size());
					
					Json::Value jscale;
					gather_keys<glm::vec3, glm::vec3>(jscale, data->scale.keys);
					
					Json::Value jrotation;
					gather_keys<glm::quat, glm::quat>(jrotation, data->rotation.keys);
					
					Json::Value jtranslation;
					gather_keys<glm::vec3, glm::vec3>(jtranslation, data->translation.keys);
					
					jnode["name"] = bone->name.c_str();
					jnode["scale"] = jscale;
					jnode["rotation"] = jrotation;
					jnode["translation"] = jtranslation;
					jnodes.append(jnode);
				}
				
			}
			janimation["nodes"] = jnodes;

			
			Json::StyledWriter writer;
			std::string buffer = writer.write(janimation);
						
			std::string abs_file_path = abs_base_path + ".animation";
			LOGV("writing animation '%s'\n", abs_file_path.c_str());
			xfile_t handle = xfile_open(abs_file_path.c_str(), XF_WRITE);
			if (xfile_isopen(handle))
			{
				xfile_write(handle, &buffer[0], buffer.length(), 1);
				xfile_close(handle);
			}
			else
			{
				LOGE("Unable to open file for writing '%s'\n", abs_file_path.c_str());
			}
		}
	}
	
	JsonModelWriter::JsonModelWriter() : write_rotations_as_quaternions(RotationIsQuaternion<RotationOutputType>())
	{
	}
	
	void JsonModelWriter::jsonify_matrix(Json::Value& array, glm::mat4& matrix)
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
	
	void JsonModelWriter::append_material(const datamodel::Material& material, Json::Value& jmaterials)
	{
		Json::Value jmaterial;
		
		jmaterial["name"] = material.name.c_str();
		jmaterial["id"] = material.id;
		
		jmaterials.append(jmaterial);
	}
	
	void JsonModelWriter::append_node(datamodel::Node* node, Json::Value& jnodes)
	{
		Json::Value jnode;
		
		jnode["name"] = node->name.c_str();
		jnode["type"] = node->type.c_str();
		
		Json::Value jscale;
		jscale.append(node->scale.x);
		jscale.append(node->scale.y);
		jscale.append(node->scale.z);
		jnode["scaling"] = jscale;
		
		Json::Value jrotation;
		if (write_rotations_as_quaternions)
		{
			glm::quat q = glm::quat(node->rotation);
			jrotation.append(q.x);
			jrotation.append(q.y);
			jrotation.append(q.z);
			jrotation.append(q.w);
		}
		else
		{
			jrotation.append(node->rotation.x);
			jrotation.append(node->rotation.y);
			jrotation.append(node->rotation.z);
		}
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
			
			
			// write blend weights
			Json::Value blend_weights(Json::arrayValue);
			for (size_t index_id = 0; index_id < node->mesh->indices.size(); ++index_id)
			{
				// each index will have an array of weight data.
				// this weight data will map bone name (by string) to value
				// at a max of MAX_SUPPORTED_BONE_INFLUENCES.
				Json::Value weight_array(Json::arrayValue);
				datamodel::WeightList& weightlist = node->mesh->weights[index_id];
				
				for (size_t influence = 0; influence < datamodel::MAX_SUPPORTED_BONE_INFLUENCES; ++influence)
				{
					datamodel::Weight& weight = weightlist.weights[influence];
					if (!weight.bone_name.empty() && weight.value > 0.0f)
					{
						Json::Value weightpair;
						weightpair["bone"] = weight.bone_name.c_str();
						weightpair["value"] = weight.value;
						weight_array.append(weightpair);
					}
				}
				
				blend_weights.append(weight_array);
			}
			
			mesh_data["blend_weights"] = blend_weights;
			
			mesh_data["material_id"] = node->mesh->material;
			
			Json::Value mass_center_offset(Json::arrayValue);
			mass_center_offset.append(node->mesh->mass_center_offset.x);
			mass_center_offset.append(node->mesh->mass_center_offset.y);
			mass_center_offset.append(node->mesh->mass_center_offset.z);
			mesh_data["mass_center_offset"] = mass_center_offset;
			
			jnode["mesh"] = mesh_data;
		}
		
		// add this node
		jnodes.append(jnode);
	}


	void JsonModelWriter::write(const std::string& abs_base_path, datamodel::Model* model)
	{
		Json::Value jroot;
		Json::Value jnodes(Json::arrayValue);
		
		if (model->export_flags == 0)
		{
			LOGW("Instructed not to export anything. wat?\n");
			assert(0);
			return;
		}

		Json::Value jmaterials(Json::arrayValue);
		if (model->export_flags & Model::EXPORT_MATERIALS)
		{
			// write out all materials
			for (const auto& material : model->materials)
			{
				append_material(material, jmaterials);
			}
		}
		jroot["materials"] = jmaterials;

		if (model->export_flags & Model::EXPORT_MESHES)
		{
			for (auto child : model->root.children)
			{
				if (child->type == "skeleton")
					continue;
				append_node(child, jnodes);
			}
		}
		jroot["nodes"] = jnodes;

		
		// write out the skeleton; if one exists
		Json::Value jskeleton(Json::arrayValue);
		if (model->skeleton && (model->export_flags & Model::EXPORT_SKELETON))
		{
			Json::Value bone_entry;
			for (const datamodel::Bone* bone : model->skeleton->bones)
			{
				bone_entry["name"] = bone->name.c_str();
				bone_entry["parent"] = bone->parent;
				Json::Value inverse_bind_pose;
				const float* data = (const float*)glm::value_ptr(bone->inverse_bind_pose);
				for (size_t i = 0; i < 16; ++i)
				{
					inverse_bind_pose.append(data[i]);
				}
				bone_entry["inverse_bind_pose"] = inverse_bind_pose;
				jskeleton.append(bone_entry);
			}
		}
		jroot["skeleton"] = jskeleton;

		
		if (model->export_flags & (Model::EXPORT_MESHES | Model::EXPORT_MATERIALS | Model::EXPORT_SKELETON))
		{
			Json::StyledWriter writer;
			
			std::string buffer = writer.write(jroot);
			
			std::string abs_model_file = abs_base_path;
			abs_model_file.append(".model");
			
			xfile_t out = xfile_open(abs_model_file.c_str(), XF_WRITE);
			LOGV("writing model: '%s'\n", abs_model_file.c_str());
			if (xfile_isopen(out))
			{
				xfile_write(out, &buffer[0], buffer.length(), 1);
				xfile_close(out);
			}
			else
			{
				LOGE("error writing to file %s\n", abs_model_file.c_str());
			}
		}

		
		if (!model->animations.empty() && (model->export_flags & Model::EXPORT_ANIMATIONS))
		{
			write_animations(abs_base_path, model);
		}
	} // write
} // namespace gemini
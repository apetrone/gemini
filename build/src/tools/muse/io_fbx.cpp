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

#include "io_fbx.h"
#include "common.h"
#include "datamodel/mesh.h"
#include "datamodel/material.h"
#include "datamodel/animation.h"

#include <core/stackstring.h>
#include <runtime/logging.h>

#include <platform/platform.h> // for MAX_PATH_SIZE

#include <fbxsdk.h>
#include <fbxsdk/utils/fbxgeometryconverter.h>

// http://gamedev.stackexchange.com/questions/59419/c-fbx-animation-importer-using-the-fbx-sdk

// http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx

// http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html
// http://www.gamedev.net/topic/653502-useful-things-you-might-want-to-know-about-fbxsdk/?hl=%2Bfbx+%2Bsdk#entry5149612
// http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/how-to-work-with-fbx-sdk-r3582
// http://forums.autodesk.com/t5/fbx-sdk/how-to-get-uv-coord-per-vertex/td-p/4239532
// http://forums.autodesk.com/t5/fbx-sdk/splitmeshespermaterial-reached-limits/td-p/4239623

// http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx

using namespace gemini::tools;

using namespace core;

namespace gemini
{
	namespace internal
	{
		static FbxManager* _manager;

	}








	void from_fbx(glm::vec3& output, const FbxVector4& input)
	{
		output = glm::vec3(input[0], input[1], input[2]);
	}

	void from_fbx(glm::quat& output, const FbxQuaternion& input)
	{
		output = glm::quat(input[3], input[0], input[1], input[2]);
	}


	void from_fbx(glm::mat4& output, const FbxAMatrix& inputmatrix)
	{
		const double* m = static_cast<const double*>(inputmatrix);
		output = glm::mat4(
			m[0], m[1], m[2], m[3],
			m[4], m[5], m[6], m[7],
			m[8], m[9], m[10], m[11],
		    m[12], m[13], m[14], m[15]);
	}

	static int get_layer_element_index(
								FbxLayerElement::EMappingMode mapping_mode,
								FbxLayerElement::EReferenceMode reference_mode,
								FbxLayerElementArrayTemplate<int>& index_array,
								int polygon_vertex,
								int vertex_id)
	{
		if (mapping_mode == FbxGeometryElement::eByControlPoint)
		{
			switch (reference_mode)
			{
				case FbxGeometryElement::eDirect:
				{
					return polygon_vertex;
					break;
				}
				case FbxGeometryElement::eIndexToDirect:
				{
					return index_array.GetAt(polygon_vertex);
					break;
				}
				case FbxGeometryElement::eIndex:
				{
					// Need to implement this
					assert(0);
					break;
				}
			}
		}
		else if (mapping_mode == FbxGeometryElement::eByPolygonVertex)
		{
			switch (reference_mode)
			{
				case FbxGeometryElement::eDirect:
				{
					return vertex_id;
					break;
				}
				case FbxGeometryElement::eIndexToDirect:
				{
					return index_array.GetAt(vertex_id);
					break;
				}
				case FbxGeometryElement::eIndex:
				{
					// Need to implement this
					assert(0);
					break;
				}
			}
		}

		return -1;
	}

	static void parse_materials(AutodeskFbxExtensionState& state, FbxNode* node, datamodel::MaterialMap& materials, datamodel::Mesh* mesh)
	{
		// materials for this piece of geometry
		int total_materials = node->GetMaterialCount();
		LOGV("%stotal_materials: %i\n", state.indent.indent(), total_materials);

		if (total_materials > 0)
		{
			for (int material_id = 0; material_id < total_materials; ++material_id)
			{
				FbxSurfaceMaterial* material = node->GetMaterial(material_id);
				LOGV("%smaterial: %s\n", state.indent.indent(), material->GetName());

				if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
				{
					LOGV("%sfound lambert material!\n", state.indent.indent());
				}
				else if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
				{
					LOGV("%sfound phong material!\n", state.indent.indent());
				}

				int texture_index;
				FBXSDK_FOR_EACH_TEXTURE(texture_index)
				{
					FbxProperty property = material->FindProperty(FbxLayerElement::sTextureChannelNames[texture_index]);
					if (property.IsValid())
					{
						int texture_count = property.GetSrcObjectCount<FbxTexture>();
						if (texture_count > 0)
						{
							LOGV("%stexture_count = %i\n", state.indent.indent(), texture_count);
							for (int texture_id = 0; texture_id < texture_count; ++texture_id)
							{
								// check if it's a layered texture
								FbxLayeredTexture* layered_texture = property.GetSrcObject<FbxLayeredTexture>(texture_id);
								if (layered_texture)
								{
									LOGV("%sfound a layered texture!\n", state.indent.indent());
									assert(0);
								}
								else
								{
									FbxTexture* texture = property.GetSrcObject<FbxTexture>(texture_id);
									if (texture)
									{
										LOGV("%stexture name: %s\n", state.indent.indent(), texture->GetName());
										LOGV("%sproperty name: %s\n", state.indent.indent(), property.GetName().Buffer());

										FbxFileTexture* file_texture = FbxCast<FbxFileTexture>(texture);
										FbxProceduralTexture* procedural_texture = FbxCast<FbxProceduralTexture>(texture);
										if (file_texture)
										{
											StackString<MAX_PATH_SIZE> texture_path = file_texture->GetFileName();
											LOGV("%stexture basename: %s\n", state.indent.indent(), texture_path.basename()());
											LOGV("%sfile name: %s, relative filename: %s\n", state.indent.indent(), file_texture->GetFileName(), file_texture->GetRelativeFileName());

											String texture_name = texture_path.basename().remove_extension()();
											String internal_material_name = "materials/" + texture_name;
											const datamodel::Material& material = materials.find_with_name(internal_material_name);

											// If you hit this assert, the mesh has multiple materials.
											// Each mesh should only have one material
											assert(mesh->material == 0);

											if (material.id == -1)
											{
												datamodel::Material newmaterial = materials.add_material(internal_material_name);
												LOGV("%sadded material: \"%s\" at index: %i\n", state.indent.indent(), texture_name.c_str(), newmaterial.id);

												mesh->material = newmaterial.id;
											}
											else
											{
												LOGV("%sfound material: \"%s\" at index: %i\n", state.indent.indent(), texture_name.c_str(), material.id);
												mesh->material = material.id;
											}

										}
										else if (procedural_texture)
										{
											LOGV("%sdetected procedural texture!\n", state.indent.indent());
											assert(0);
										}



	//									LOGV("%sscale u: %g\n", state.indent(), texture->GetScaleU());
	//									LOGV("%sscale v: %g\n", state.indent(), texture->GetScaleV());
	//									LOGV("%stranslation u: %g\n", state.indent(), texture->GetTranslationU());
	//									LOGV("%stranslation v: %g\n", state.indent(), texture->GetTranslationV());
	//									LOGV("%sswap uv: %i\n", state.indent(), texture->GetSwapUV());
	//									LOGV("%srotation u: %g\n", state.indent(), texture->GetRotationU());
	//									LOGV("%srotation v: %g\n", state.indent(), texture->GetRotationV());
	//									LOGV("%srotation w: %g\n", state.indent(), texture->GetRotationW());
									}
								}
							}
						}
					}
				}
			}
		}
	}

	static void load_mesh(AutodeskFbxExtensionState& state, FbxNode* node, datamodel::Mesh* mesh)
	{
		FbxMesh* fbxmesh = node->GetMesh();

		parse_materials(state, node, state.model->materials, mesh);

		fbxmesh->GenerateNormals();
		fbxmesh->GenerateTangentsData();

		mesh->name = node->GetName();
		LOGV("loading mesh: '%s', %p\n", node->GetName(), mesh);


		AutodeskFbxExtensionState::MeshData* mesh_data = 0;
		if (state.meshdata.has_key(mesh->name))
		{
			mesh_data = state.meshdata[mesh->name];
		}
		else
		{
			mesh_data = MEMORY_NEW(AutodeskFbxExtensionState::MeshData, core::memory::global_allocator());
			mesh_data->name = mesh->name;
			state.meshdata[mesh->name] = mesh_data;
		}


		int total_deformers = fbxmesh->GetDeformerCount();
	//	LOGV("%stotal_deformers: %i\n", state.indent(), total_deformers);

		state.indent.update();
		int total_triangles = fbxmesh->GetPolygonCount();
	//	LOGV("%s# triangles: %i\n", state.indent(), total_triangles);
	//	LOGV("%s# vertices: %i\n", state.indent(), fbxmesh->GetControlPointsCount());

		size_t total_indices = fbxmesh->GetPolygonCount()*3;

		mesh->indices.allocate(total_indices);
		mesh->vertices.allocate(total_indices);
		mesh->normals.allocate(total_indices);
	//	LOGV("%s# indices: %i\n", state.indent(), total_indices);

		datamodel::Vertex* vertices = MEMORY_NEW_ARRAY(datamodel::Vertex, total_indices, core::memory::global_allocator());
		size_t vertex_index = 0;
		size_t local_index = 0;

		if (fbxmesh->GetElementVertexColorCount() > 0)
		{
			mesh->vertex_colors.allocate(total_indices);
		}

		// uv coordinates
		FbxStringList uv_set_list;
		fbxmesh->GetUVSetNames(uv_set_list);
	//	LOGV("%stotal_uv_sets: %i\n", state.indent(), uv_set_list.GetCount());


		// clamp the # of maximum uv sets
		int total_uv_sets = uv_set_list.GetCount();
		if (total_uv_sets > datamodel::MAX_SUPPORTED_UV_CHANNELS)
		{
			LOGW("%stotal uv sets exceeds MAX_SUPPORTED_UV_CHANNELS (%i)\n", state.indent.indent(), datamodel::MAX_SUPPORTED_UV_CHANNELS);

			// print out the sets being ignored
			state.indent.push();
			for (int item_index = datamodel::MAX_SUPPORTED_UV_CHANNELS; item_index < total_uv_sets; ++item_index)
			{
				FbxStringListItem* item = uv_set_list.GetItemAt(item_index);
				LOGW("%signoring set: \"%s\"\n", state.indent.indent(), item->mString.Buffer());
			}
			state.indent.pop();
			total_uv_sets = datamodel::MAX_SUPPORTED_UV_CHANNELS;
		}

		mesh->uvs.allocate(total_uv_sets);
		for (int uvset = 0; uvset < total_uv_sets; ++uvset)
		{
			mesh->uvs[uvset].allocate(total_indices);
		}

		FbxGeometryElementVertexColor* vertex_colors = 0;
		if (fbxmesh->GetElementVertexColorCount() > 0)
		{
			vertex_colors = fbxmesh->GetElementVertexColor(0);

			// At the moment, we do not handle more than a single Vertex Color layer
			assert(fbxmesh->GetElementVertexColorCount() <= 1);
		}

		// allocate weights
		mesh->weights.allocate(total_indices);

		for (int triangle_index = 0; triangle_index < total_triangles; ++triangle_index)
		{
			for (int local_vertex_id = 0; local_vertex_id < 3; ++local_vertex_id)
			{
				int index = fbxmesh->GetPolygonVertex(triangle_index, local_vertex_id);
//				LOGV("%sindex: %i [vertex_index = %i]\n", state.indent.indent(), index, vertex_index);
				mesh->indices[local_index] = local_index;

				const FbxVector4& position = fbxmesh->GetControlPointAt(index);
//				LOGV("%svertex: %g %g %g\n", state.indent.indent(), position[0], position[1], position[2]);

				FbxVector4 normal;
				fbxmesh->GetPolygonVertexNormal(triangle_index, local_vertex_id, normal);
	//			LOGV("%snormal: %g %g %g\n", state.indent(), normal[0], normal[1], normal[2]);

				datamodel::Vertex* vertex = &vertices[vertex_index];

				// MUSE_FBX_UNIT_FIX
				vertex->position = glm::vec3(position[0], position[1], position[2]) * state.conversion_factor;
				vertex->normal = glm::vec3(normal[0], normal[1], normal[2]);
				for (int uvset = 0; uvset < total_uv_sets; ++uvset)
				{
					FbxVector2 uv;
					bool unmapped;
					FbxStringListItem* item = uv_set_list.GetItemAt(uvset);
					bool has_uv = fbxmesh->GetPolygonVertexUV(triangle_index, local_vertex_id, item->mString, uv, unmapped);
					if (!has_uv)
					{
						LOGE("%sUnable to get polygon vertex at [t=%i, v=%i, set=%s\n", state.indent.indent(), triangle_index, local_vertex_id, item->mString.Buffer());
					}
		//			LOGV("%suv: %g %g\n", state.indent(), uv[0], uv[1]);
					vertex->uv[uvset] = glm::vec2(uv[0], uv[1]);
					mesh->uvs[uvset][vertex_index] = vertex->uv[uvset];
				}

				if (vertex_colors)
				{
					int vertex_color_id = get_layer_element_index(vertex_colors->GetMappingMode(), vertex_colors->GetReferenceMode(), vertex_colors->GetIndexArray(), triangle_index, local_index);
					const FbxColor& color = vertex_colors->GetDirectArray().GetAt(vertex_color_id);
					//LOGV("%scolor: %2.2f %2.2f %2.2f %2.2f\n", state.indent(), color.mRed, color.mGreen, color.mBlue, color.mAlpha);
					vertex->color = glm::vec4(color.mRed, color.mGreen, color.mBlue, color.mAlpha);
				}

				mesh->vertices[vertex_index] = vertex->position;
				mesh->normals[vertex_index] = vertex->normal;

				if (!mesh->vertex_colors.empty())
				{
					mesh->vertex_colors[vertex_index] = vertex->color;
				}


				WeightSlotVector* slots = &mesh_data->weight_slots;
				if (!slots->empty())
				{

					datamodel::WeightList& weightlist = mesh->weights[vertex_index];
					state.indent.push();
					// copy weights

					for (WeightReference& weight_ref : (*slots)[index].weights)
					{
	//					LOGV("%sbone = %i, weight = %2.2f\n", state.indent.indent(), weight_ref.datamodel_bone_index, weight_ref.value);

						// assert if we hit this limit -- can probably just drop anything
						// over the max, but we should be smart -- and re-normalize.
						assert(weightlist.total_weights <= datamodel::MAX_SUPPORTED_BONE_INFLUENCES);

						// copy weight data over
						datamodel::Weight& weight = weightlist.weights[weightlist.total_weights++];
						datamodel::Bone* bone = state.model->skeleton->get_bone_at_index(weight_ref.datamodel_bone_index);
						assert(bone != nullptr);
						weight.bone_name = bone->name;
						weight.value = weight_ref.value;
					}
					state.indent.pop();
				}


				++vertex_index;
				++local_index;
			}
		}


		// copy bone data
		if (!mesh_data->bones.empty())
		{
			mesh->bindpose.allocate(mesh_data->bones.size());

			size_t index = 0;
			for (auto& bd : mesh_data->bones)
			{
				datamodel::BoneLinkData& link_data = mesh->bindpose[index++];
				datamodel::Bone* bone = state.model->skeleton->find_bone_named(bd.name);
				link_data.bone_name = bd.name;
				link_data.inverse_bind_pose = bd.inverse_bind_pose;
				link_data.parent = bone->parent;
			}
		}


	//	LOGV("%svertex_index: %i\n", state.indent(), vertex_index);
		MEMORY_DELETE_ARRAY(vertices, core::memory::global_allocator());
	}


	static String type_from_node(FbxNode* node)
	{
		if (node->GetSkeleton())
		{
			return "skeleton";
		}
		else if (node->GetMesh())
		{
			return "mesh";
		}
		else
		{
			return "node";
		}
	}

	static void to_mat4(FbxAMatrix& tr, glm::mat4& out)
	{

	}

	bool is_hierarchical_node(FbxNode* node)
	{
		// doesn't need to traverse into children of this node
		// or create it as part of the scene graph for transforms, etc.
		// One major annoyance here is:
		// if a user exports to FBX but unchecks 'Lights' or 'Cameras';
		// they will still be exported, but will just not be flagged as their type.
		return !(node->GetLight() || node->GetCamera());
	}

	static void populate_animations(AutodeskFbxExtensionState& state, FbxNode* fbxnode, FbxTakeInfo* take, FbxTime::EMode time_mode, datamodel::Animation& animation)
	{
		FbxTime start = take->mLocalTimeSpan.GetStart();
		FbxTime end = take->mLocalTimeSpan.GetStop();
		int frame_count = end.GetFrameCount(time_mode) - start.GetFrameCount(time_mode) + 1;
	//	LOGV("%stotal frames: %i\n", state.indent(), frame_count);

		state.indent.push();

		bool is_hierarchical = is_hierarchical_node(fbxnode);

		String node_name = fbxnode->GetName();
//		datamodel::Node* node = state.model->root.find_child_named(node_name);
		datamodel::Bone* bone = state.model->skeleton->find_bone_named(node_name);

		if (is_hierarchical && bone)
		{
	//	if (!node)
	//	{
	//		LOGW("%sCould not find node named \"%s\"\n", state.indent(), node_name.c_str());
	//		return;
	//	}
			LOGV("%snode: %s\n", state.indent.indent(), node_name.c_str());

			datamodel::NodeAnimation node_data;

			datamodel::NodeAnimation* data = animation.data_with_name(node_name);

			animation.frames_per_second = state.frames_per_second;

			// we shouldn't run into the event where we're adding multiple animation data
			// to the same node name
			assert(!data);
			if (!data)
			{
				data = animation.add_node_data(node_name);
			}

			animation.duration_seconds = end.GetFrameCount(time_mode)/state.frames_per_second;

			FbxQuaternion last_rotation;

			for (FbxLongLong local_time = start.GetFrameCount(time_mode); local_time <= end.GetFrameCount(time_mode); ++local_time)
			{
				FbxTime current_time;
				current_time.SetFrame(local_time, time_mode);

				float frame_time = current_time.GetSecondDouble();

				// Unfortunately, EvaluateLocalTransform doesn't do what I expected.
				// So we have to generate the local transforms from the global transforms.

				const FbxAMatrix& global_transform = fbxnode->EvaluateGlobalTransform(current_time);

				FbxAMatrix transform = global_transform;
				FbxVector4 scaling = fbxnode->EvaluateLocalScaling(current_time);
				FbxVector4 translation = fbxnode->EvaluateLocalTranslation(current_time);

				FbxQuaternion rotation;

				if (bone->parent != -1)
				{
					FbxNode* parent = state.skeletal_nodes[bone->parent];
					assert(parent != 0);

					FbxAMatrix parent_global_transform = parent->EvaluateGlobalTransform(current_time);
					FbxAMatrix local_transform = parent_global_transform.Inverse() * global_transform;
					scaling = local_transform.GetS();
					translation = local_transform.GetT();
					rotation = local_transform.GetQ();
				}
				else
				{
					scaling = global_transform.GetS();
					translation = global_transform.GetT();
					rotation = global_transform.GetQ();
				}

				// We need to check if the quaternion sign has flipped
				// otherwise we'll end up with rapid opposite rotations in the animation.
				if (last_rotation.DotProduct(rotation) < 0)
				{
					rotation = -rotation;
				}
				last_rotation = rotation;


//				LOGV("s: %g %g %g\n", scaling[0], scaling[1], scaling[2]);
//				LOGV("r: %2.2f, %2.2f, %2.2f, %2.2f\n", rotation[3], rotation[0], rotation[1], rotation[2]);
//				LOGV("t: %g %g %g\n", translation[0], translation[1], translation[2]);

				glm::vec3 key_scaling;
				glm::quat key_rotation;
				glm::vec3 key_translation;

				from_fbx(key_scaling, scaling);
				// MUSE_FBX_UNIT_FIX
				key_scaling *= 1.0f/state.conversion_factor;

				from_fbx(key_rotation, rotation);
				from_fbx(key_translation, translation);

				// convert quaternion to euler angles for the datamodel
//				glm::vec3 rotation_degrees = glm::eulerAngles(key_rotation);
//				rotation_degrees.x = mathlib::radians_to_degrees(rotation_degrees.x);
//				rotation_degrees.y = mathlib::radians_to_degrees(rotation_degrees.y);
//				rotation_degrees.z = mathlib::radians_to_degrees(rotation_degrees.z);

				data->scale.add_key(frame_time, key_scaling);
				data->rotation.add_key(frame_time, key_rotation);
				data->translation.add_key(frame_time, key_translation);
			}
		}
		else
		{
			LOGV("skip: %s\n", fbxnode->GetName());
		}

		for (int index = 0; index < fbxnode->GetChildCount(); ++index)
		{
			populate_animations(state, fbxnode->GetChild(index), take, time_mode, animation);
		}

		state.indent.pop();
	}

	static void process_blendweights(AutodeskFbxExtensionState& state, FbxNode* fbxnode)
	{
		// If you hit this, there is NO skeleton found and we're trying to read
		// blendweights.
		assert(state.model->skeleton);

		if (fbxnode->GetMesh())
		{
			FbxMesh* fbxmesh = fbxnode->GetMesh();
			assert(fbxmesh != 0);

			int total_deformers = fbxmesh->GetDeformerCount();

			FbxVector4 scale = fbxnode->GetGeometricScaling(FbxNode::eSourcePivot);
			FbxVector4 rotation = fbxnode->GetGeometricRotation(FbxNode::eSourcePivot);
			FbxVector4 translation = fbxnode->GetGeometricTranslation(FbxNode::eSourcePivot);

			FbxAMatrix geometry_transform(translation, rotation, scale);

			AutodeskFbxExtensionState::MeshData* mesh_data = 0;
			if (state.meshdata.has_key(fbxnode->GetName()))
			{
				mesh_data = state.meshdata[fbxnode->GetName()];
			}
			else
			{
				mesh_data = MEMORY_NEW(AutodeskFbxExtensionState::MeshData, core::memory::global_allocator());
				mesh_data->name = fbxnode->GetName();
				state.meshdata[fbxnode->GetName()] = mesh_data;
			}

			WeightSlotVector *slots = &mesh_data->weight_slots;


			// This makes the assumption that there will ONLY ever be a single mesh
			// linked to a single skeleton.
			for(int deformer_index = 0; deformer_index < total_deformers; ++deformer_index)
			{
				FbxDeformer* deformer = fbxmesh->GetDeformer(deformer_index);

				if (deformer->GetDeformerType() == FbxDeformer::eSkin)
				{
					FbxSkin* skin = reinterpret_cast<FbxSkin*>(deformer);

					LOGV("resizing vector to %i control points (indices)\n", fbxmesh->GetControlPointsCount());
					slots->resize(fbxmesh->GetControlPointsCount());


					int total_clusters = skin->GetClusterCount();
					LOGV("total clusters: %i\n", total_clusters);

					// TODO: When exporting from maya, the local rotation axes for joints
					// must be 'to world', because that's how we currently assume rotations
					// are exported.

					// There is ONE known issue with this:
					// It will only calculate the inverse bind pose for bones that are present
					// as a cluster -- that is; it must be bound to some verts in the mesh.

					for (int cluster_index = 0; cluster_index < total_clusters; ++cluster_index)
					{
						FbxCluster* cluster = skin->GetCluster(cluster_index);
						LOGV("cluster: %i, joint name: %s\n", cluster_index, cluster->GetLink()->GetName());
						datamodel::Bone* bone = state.model->skeleton->find_bone_named(cluster->GetLink()->GetName());
						BoneData& bonedata = mesh_data->bones[cluster->GetLink()->GetName()];
						bonedata.name = cluster->GetLink()->GetName();

						// If you hit this, lookup failed for the cluster name.

						assert(bone != 0);

						FbxAMatrix transform_matrix;
						FbxAMatrix link_matrix;
						FbxAMatrix inverse_bindpose_matrix;



						cluster->GetTransformMatrix(transform_matrix);
						cluster->GetTransformLinkMatrix(link_matrix);

						inverse_bindpose_matrix = link_matrix.Inverse() * transform_matrix * geometry_transform;
						from_fbx(bonedata.inverse_bind_pose, inverse_bindpose_matrix);


						int total_control_points = cluster->GetControlPointIndicesCount();
						int* indices = cluster->GetControlPointIndices();
						double* control_point_weights = cluster->GetControlPointWeights();
						int control_point_index = 0;
						for(; control_point_index < total_control_points; ++control_point_index)
						{
//							LOGV("%i --> %2.2f\n", control_point_index, control_point_weights[control_point_index]);

							WeightReference ref;
							ref.datamodel_bone_index = bone->index;
							ref.value = control_point_weights[control_point_index];
							(*slots)[indices[control_point_index]].weights.push_back(ref);
							bone->total_blendweights++;
						}
					}
				}
				else if (deformer->GetDeformerType() == FbxDeformer::eBlendShape)
				{
					LOGV("Unable to read blend shapes! Fix me.\n");
					assert(0);
				}
				else
				{
					assert(0);
					LOGV("Unknown deformer type!\n");
				}
			}
		}


		for (int index = 0; index < fbxnode->GetChildCount(); ++index)
		{
			process_blendweights(state, fbxnode->GetChild(index));
		}
	}

	static void process_skeleton(AutodeskFbxExtensionState& state, int32_t parent_index, FbxNode* fbxnode)
	{
		// I'm assuming that the skeleton has no parent.
		FbxNodeAttribute::EType node_attribute_type = fbxnode->GetNodeAttribute()->GetAttributeType();


		// in the event there's a transform between skeletal nodes
		// (because of mirroring, for example)
		if (node_attribute_type == FbxNodeAttribute::eSkeleton)
		{
			datamodel::Bone* bone = state.model->skeleton->add_bone(parent_index, fbxnode->GetName());

//			LOGV("%sprocess_skeleton: [parent_index=%i, index=%i, name=%s]\n", state.indent.indent(), parent_index, bone->index, fbxnode->GetName());
			parent_index = bone->index;
			state.skeletal_nodes.push_back(fbxnode);
		}

		for (int index = 0; index < fbxnode->GetChildCount(); ++index)
		{
			state.indent.push();
			process_skeleton(state, parent_index, fbxnode->GetChild(index));
			state.indent.pop();
		}
	}

	static void populate_hierarchy(AutodeskFbxExtensionState& state, datamodel::Node* root, FbxNode* fbxnode)
	{
		state.indent.push();

		if (is_hierarchical_node(fbxnode))
		{
			// determine which order the rotation and scaling operations are performed...
//			FbxTransform::EInheritType transform_inherit_type;
//			fbxnode->GetTransformationInheritType(transform_inherit_type);
//
//			StackString<64> inherit_type;
//			switch(transform_inherit_type)
//			{
//				case FbxTransform::eInheritRrSs:
//					inherit_type = "RrSs\n";
//					break;
//				case FbxTransform::eInheritRSrs:
//					inherit_type = "RSrs";
//					break;
//				case FbxTransform::eInheritRrs:
//					inherit_type = "Rrs";
//					break;
//			}
//
//			LOGV("Node inherit type: %s\n", inherit_type());

			// create a new node
			datamodel::Node* node = MEMORY_NEW(datamodel::Node, core::memory::global_allocator());
			root->add_child(node);

			node->name = fbxnode->GetName();
			LOGV("-------------- %s --------------\n", node->name.c_str());

			NodeData nodedata;
			nodedata.node = node;
			state.nodedata.push_back(nodedata);

			FbxNodeAttribute::EType node_attribute_type = fbxnode->GetNodeAttribute()->GetAttributeType();


			// copy data
			node->type = type_from_node(fbxnode);
			if (node_attribute_type == FbxNodeAttribute::eMesh)
			{
				node->mesh = MEMORY_NEW(datamodel::Mesh, core::memory::global_allocator());
				// populate mesh from fbxnode
				state.indent.push();
				load_mesh(state, fbxnode, node->mesh);
				state.indent.pop();
			}
			else if (node_attribute_type == FbxNodeAttribute::eSkeleton)
			{
				datamodel::Bone* bone = state.model->skeleton->find_bone_named(fbxnode->GetName());
				assert(bone != 0);

				if (bone->total_blendweights > 0)
				{
					node->flags |= datamodel::Node::HasAnimations;
				}
			}
			else
			{
				LOGW("Node type ignored: %s, name = %s\n", node->type.c_str(), fbxnode->GetName());
			}

			FbxAMatrix& local_transform = fbxnode->EvaluateLocalTransform();
			const FbxDouble3& local_scale = local_transform.GetS();
			const FbxQuaternion& local_rotation = local_transform.GetQ();
			const FbxDouble3& local_translation = local_transform.GetT();
//			LOGV("ls: %g %g %g\n", local_scale[0], local_scale[1], local_scale[2]);
//			LOGV("lr: %g %g %g\n", local_rotation[0], local_rotation[1], local_rotation[2]);
//			LOGV("lt: %g %g %g\n", local_translation[0], local_translation[1], local_translation[2]);

			from_fbx(node->scale, local_scale);

			// MUSE_FBX_UNIT_FIX
			node->scale *= (1/state.conversion_factor);

			from_fbx(node->rotation, local_rotation);
			from_fbx(node->translation, local_translation);

			// if this node isn't a skeleton; then we traverse the hierarchy
			if (node_attribute_type != FbxNodeAttribute::eSkeleton)
			{
				for (int index = 0; index < fbxnode->GetChildCount(); ++index)
				{
					populate_hierarchy(state, node, fbxnode->GetChild(index));
				}
			}
		}

		state.indent.pop();
	}

	AutodeskFbxReader::AutodeskFbxReader()
	{
		internal::_manager = FbxManager::Create();

		LOGV("FBX SDK %s\n", FbxManager::GetVersion());

		int sdk_version_major;
		int sdk_version_minor;
		int sdk_version_patch;

		FbxManager::GetFileFormatVersion(sdk_version_major, sdk_version_minor, sdk_version_patch);
		LOGV("FBX SDK default file version: %i.%i.%i\n", sdk_version_major, sdk_version_minor, sdk_version_patch);
	}

	AutodeskFbxReader::~AutodeskFbxReader()
	{
		internal::_manager->Destroy();
	}

	float fps_from_timemode(FbxTime::EMode mode)
	{
		switch(mode)
		{
			case FbxTime::eFrames120: return 120;
			case FbxTime::eFrames100: return 100;
			case FbxTime::eFrames60: return 60;
			case FbxTime::eFrames50: return 50;
			case FbxTime::eFrames48: return 48;
			case FbxTime::eFrames30: // fall through
			case FbxTime::eFrames30Drop: // fall through
			case FbxTime::eNTSCDropFrame: // fall through
			case FbxTime::eNTSCFullFrame: return 30;
			case FbxTime::ePAL: return 25;
			case FbxTime::eFrames24: return 24;
			case FbxTime::eFrames1000: return 0.0f;
			case FbxTime::eFilmFullFrame: return 24;
			case FbxTime::eCustom: assert(0); // not implemented!
			case FbxTime::eFrames96: return 96;
			case FbxTime::eFrames72: return 72;
			case FbxTime::eFrames59dot94: return 60;
			default: break;
		}

		return 0.0f;
	}

	bool AutodeskFbxReader::find_skeleton(FbxNode* node)
	{
		// Nodes exported by blender do not have
		// a proper NodeAttribute when using FBX 7.4 binary.
		if (node->GetNodeAttribute())
		{
			FbxNodeAttribute::EType node_attribute_type = node->GetNodeAttribute()->GetAttributeType();
			if (node_attribute_type == FbxNodeAttribute::eSkeleton)
			{
				// If you hit this assert, there's another skeleton at the root level.
				// At the moment, only a single skeleton is supported.
//				assert(extension_state.model->skeleton == 0);

				if (!extension_state.model->skeleton)
				{
					extension_state.model->skeleton = MEMORY_NEW(datamodel::Skeleton, core::memory::global_allocator());
				}

				process_skeleton(extension_state, -1, node);
				return true;
			}
//			else
//			{
//				LOGV("found node attribute type: %i\n", node_attribute_type);
//			}
		}

//		LOGV("find_skeleton: searching %i children...\n", node->GetChildCount());
		for (int index = 0; index < node->GetChildCount(); ++index)
		{
			find_skeleton(node->GetChild(index));
		}

		return false;
	}

	bool AutodeskFbxReader::read(datamodel::Model* model, util::DataStream& data_source)
	{
		const char* path = (const char*)data_source.get_data();

		LOGV("loading model: %s\n", path);

		FbxManager* manager = internal::_manager;

		FbxIOSettings* settings = FbxIOSettings::Create(manager, IOSROOT);
		FbxImporter* importer = FbxImporter::Create(manager, "");

		settings->SetBoolProp(IMP_FBX_MATERIAL, true);
		settings->SetBoolProp(IMP_FBX_TEXTURE, true);


		if (!importer->Initialize(path, -1, settings))
		{
			LOGE("initialize exporter failed\n");
			LOGE("%s\n", importer->GetStatus().GetErrorString());
			return false;
		}

		// query and print out the detected file version
		int file_version_major;
		int file_version_minor;
		int file_version_patch;
		importer->GetFileVersion(file_version_major, file_version_minor, file_version_patch);
		LOGV("FBX File Format: %i.%i.%i\n", file_version_major, file_version_minor, file_version_patch);

		FbxScene* scene = FbxScene::Create(manager, "");

		importer->Import(scene);

		FbxGeometryConverter converter(manager);

		// triangulate the scene
		converter.Triangulate(scene, true);

		// split meshes by material
		converter.SplitMeshesPerMaterial(scene, true);

		// safe to destroy the importer after importing a scene
		importer->Destroy();

		// analyze scene
		LOGV("scene info:\n");
		LOGV("\toriginal_application: %s\n", scene->GetSceneInfo()->Original_ApplicationName.Get().Buffer());
		LOGV("\toriginal_application_vendor: %s\n", scene->GetSceneInfo()->Original_ApplicationVendor.Get().Buffer());
		LOGV("\toriginal_application_version: %s\n", scene->GetSceneInfo()->Original_ApplicationVersion.Get().Buffer());
		LOGV("\toriginal_datetime_gmt: %s\n", scene->GetSceneInfo()->Original_DateTime_GMT.Get().toString().Buffer());
		LOGV("\toriginal_filename: %s\n", scene->GetSceneInfo()->Original_FileName.Get().Buffer());

		FbxTime::EMode time_mode = scene->GetGlobalSettings().GetTimeMode();
		LOGV("\ttime mode = %s\n", FbxGetTimeModeName(time_mode));

		extension_state.frames_per_second = fps_from_timemode(time_mode);

		// this will ONLY modify the first layer of node transforms in the scene
		// also, I bet the DCC tool has to export this with the correct up-axis
		// as some are still aligned on the Z axis even though they report
		// MayaYUp.
		FbxAxisSystem axis_system = scene->GetGlobalSettings().GetAxisSystem();
		if (axis_system == FbxAxisSystem::MayaYUp)
		{
			LOGV("model matches Maya's Y-up axis. nothing to do.\n");
		}
		else
		{
			LOGV("axis_system needs to be changed\n");
			FbxAxisSystem::MayaYUp.ConvertScene(scene);
		}

		// gemini needs a scene defined in meters
		// ugh, this is stupid: http://forums.autodesk.com/t5/fbx-sdk/scene-units-for-animation-translations/td-p/4262325
		FbxSystemUnit system_unit = scene->GetGlobalSettings().GetSystemUnit();

		LOGV("scene is in unit: %s\n", system_unit.GetScaleFactorAsString().Buffer());

		if (system_unit != FbxSystemUnit::m)
		{
			LOGV("Converting scene units from: %s\n", system_unit.GetScaleFactorAsString().Buffer());
	//		FbxSystemUnit::m.ConvertScene(scene);
	//		FbxSystemUnit::m.ConvertChildren(scene->GetRootNode(), system_unit);
		}

		// assume exporting from Maya
		extension_state.conversion_factor = FbxSystemUnit::cm.GetConversionFactorTo(system_unit);
		LOGV("conversion_factor: %g\n", extension_state.conversion_factor);

	//	int total_poses = scene->GetPoseCount();
	//	LOGV("pose count: %i\n", total_poses);


		extension_state.model = model;


//		LOGV("pose count = %i\n", scene->GetPoseCount());
//
//		if (scene->GetPoseCount() > 0)
//		{
//			FbxPose* pose = scene->GetPose(0);
//			LOGV("is_bind_pose: %s\n", pose->IsBindPose() ? "Yes" : "No");
//
//			for (int p = 0; p < pose->GetCount(); ++p)
//			{
//				FbxNode* node = pose->GetNode(p);
//				LOGV("node: %i, %s\n", p, node->GetName());
//			}
//		}


		FbxNode* fbxroot = scene->GetRootNode();
		if (fbxroot)
		{
			// The fbx data must be read in the following order:
			// 1. build skeleton
			// 2. process blend weights for geometry
			// 3. read in mesh geometry
			// It is done so because we need to reference bones by name
			// and we need to have blendweights when reading in the geometry.

			// 1. build skeleton
			find_skeleton(fbxroot);

			// now process all weights
			// This must be done after hierarchy traversal to ensure all the bones
			// have been created.
			if (extension_state.model->skeleton)
			{
				for (int index = 0; index < fbxroot->GetChildCount(); ++index)
				{
					process_blendweights(extension_state, fbxroot->GetChild(index));
				}
			}

			// 3. read geometry
			for (int index = 0; index < fbxroot->GetChildCount(); ++index)
			{
				populate_hierarchy(extension_state, &model->root, fbxroot->GetChild(index));
			}


			int total_animation_stacks = scene->GetSrcObjectCount<FbxAnimStack>();
			LOGV("animation stacks: %i\n", total_animation_stacks);

			// If we need to implement more than one animation... do that now.
			// Interestingly enough, the FBX 6.1 ASCII format exports 2 stacks
			// for ... reasons.
			assert(total_animation_stacks <= 1);

			if (extension_state.model->skeleton)
			{
				for (int stack = 0; stack < total_animation_stacks; ++stack)
				{
					FbxAnimStack* anim_stack = scene->GetSrcObject<FbxAnimStack>(stack);
					FbxString stack_name = anim_stack->GetName();
					LOGV("stack: %s\n", stack_name.Buffer());
					FbxTakeInfo* take = scene->GetTakeInfo(stack_name);

					datamodel::Animation* animation = model->add_animation(stack_name.Buffer());
					LOGV("reading data for animation \"%s\"\n", animation->name.c_str());

		//			fbxroot->ConvertPivotAnimationRecursive(anim_stack, FbxNode::eDestinationPivot, 30.0);
					populate_animations(extension_state, fbxroot, take, time_mode, *animation);
				}
			}
		}

		return true;
	}
} // namespace gemini

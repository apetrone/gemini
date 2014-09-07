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

#include <fbxsdk.h>
#include <fbxsdk/utils/fbxgeometryconverter.h>

#include <gemini/core/log.h>
#include <slim/xlog.h>

#include "io_fbx.h"
#include "common.h"
#include "datamodel/mesh.h"

// http://gamedev.stackexchange.com/questions/59419/c-fbx-animation-importer-using-the-fbx-sdk

using namespace tools;

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
}

static void load_mesh2(IndentState& state, FbxNode* node, datamodel::Mesh* mesh)
{
	FbxMesh* fbxmesh = node->GetMesh();
	
	
	int total_triangles = fbxmesh->GetPolygonCount()/3;
	LOGV("%s# triangles: %i\n", state.indent(), total_triangles);
	
	for (int triangle_index = 0; triangle_index < total_triangles; ++triangle_index)
	{
		for (int vertex_index = 0; vertex_index < 3; ++vertex_index)
		{
			int index = fbxmesh->GetPolygonVertex(triangle_index, vertex_index);
			LOGV("%sindex: %i\n", state.indent(), index);
			
			const FbxVector4& vertex = fbxmesh->GetControlPointAt(index);
			LOGV("%svertex: %g %g %g\n", state.indent(), vertex[0], vertex[1], vertex[2]);
			
			FbxVector4 normal;
			fbxmesh->GetPolygonVertexNormal(triangle_index, vertex_index, normal);
			LOGV("%snormal: %g %g %g\n", state.indent(), normal[0], normal[1], normal[2]);
			
			FbxVector2 uv;
			bool unmapped;
			fbxmesh->GetPolygonVertexUV(triangle_index, vertex_index, "UVMap", uv, unmapped);
			LOGV("%suv: %g %g\n", state.indent(), uv[0], uv[1]);
		}
	}
}

static void load_mesh(IndentState& state, FbxNode* node, datamodel::Mesh* mesh)
{
	return load_mesh2(state, node, mesh);
	
	FbxMesh* fbxmesh = node->GetMesh();
	
	int total_layers = fbxmesh->GetLayerCount();
	LOGV("total layers: %i\n", total_layers);
	//		assert(total_layers == 1);
	
	
	
	int bad_polys_removed = fbxmesh->RemoveBadPolygons();
	if (bad_polys_removed > 0)
	{
		LOGV("bad_polys_removed = %i\n", bad_polys_removed);
	}
	
	fbxmesh->GenerateTangentsData();
	
	
	int total_deformers = fbxmesh->GetDeformerCount();
	LOGV("total_deformers: %i\n", total_deformers);
	
	FbxLayerElementNormal* normals = nullptr;
	FbxLayer* normal_layer = fbxmesh->GetLayer(0, FbxLayerElement::eNormal);
	if (normal_layer)
	{
		normals = normal_layer->GetNormals();
	}
	else
	{
		LOGW("No normals present; generating normals...\n");
		fbxmesh->GenerateNormals();
		normal_layer = fbxmesh->GetLayer(0, FbxLayerElement::eNormal);
		assert(normal_layer != 0);
		if (normal_layer)
		{
			normals = normal_layer->GetNormals();
		}
	}
	
	
	
	FbxLayerElementVertexColor* colors = fbxmesh->GetLayer(0)->GetVertexColors();
	
	FbxLayerElementUV* uvs = fbxmesh->GetLayer(0)->GetUVs();
	
	
	//		FbxLayerElementBinormal* binormals = fbxmesh->GetLayer(0)->GetBinormals();
	//		FbxLayerElementTangent* tangents = fbxmesh->GetLayer(0)->GetTangents();
	
	//
	// copy uv coordinate sets
	FbxStringList uv_set_list;
	fbxmesh->GetUVSetNames(uv_set_list);
	
	LOGV("total_uv_sets: %i\n", uv_set_list.GetCount());
	
	// don't need to assert here; just need to ignore unsupported channels
	assert(uv_set_list.GetCount() <= datamodel::MAX_SUPPORTED_UV_CHANNELS);
	
	mesh->total_uv_sets = uv_set_list.GetCount();
	
	assert(normals != 0);
	
	int total_vertices = fbxmesh->GetControlPointsCount();
	LOGV("%stotal vertices: %i\n", state.indent(), total_vertices);
	
	int total_indices = fbxmesh->GetPolygonVertexCount();
	LOGV("%stotal indices: %i\n", state.indent(), total_indices);
	mesh->indices.allocate(total_indices);
	int* indices = fbxmesh->GetPolygonVertices();
	memcpy(&mesh->indices[0], indices, sizeof(int)*total_indices);
	
	
	
	mesh->vertices.allocate(total_vertices);
	mesh->normals.allocate(total_vertices);
	if (colors)
	{
		LOGV("%stotal colors: %i\n", colors->GetDirectArray().GetCount());
		mesh->vertex_colors.allocate(total_vertices);
	}
	
	if (uvs)
	{
		LOGV("%stotal uvs: %i\n", state.indent(), uvs->GetDirectArray().GetCount());
		mesh->uvs[0].allocate(total_vertices);
	}
	
	if (normals)
	{
		LOGV("%stotal normals: %i\n", state.indent(), normals->GetDirectArray().GetCount());
	}
	
	
	int total_polygons = fbxmesh->GetPolygonCount();
	int vertex_counter = 0;
	for (int poly = 0; poly < total_polygons; ++poly)
	{
		for (int v = 0; v < fbxmesh->GetPolygonSize(poly); ++v)
		{
			int vertex_position = fbxmesh->GetPolygonVertex(poly, v);
			
			FbxVector4 vertex = fbxmesh->GetControlPoints()[vertex_position];
			glm::vec3& m_vertex = mesh->vertices[vertex_position];
			m_vertex.x = vertex[0];
			m_vertex.y = vertex[1];
			m_vertex.z = vertex[2];
			
			int normal_index = get_layer_element_index(normals->GetMappingMode(), normals->GetReferenceMode(), normals->GetIndexArray(), vertex_position, vertex_counter);
			FbxVector4 normal = normals->GetDirectArray().GetAt(normal_index);
			glm::vec3& m_normal = mesh->normals[vertex_position];
			m_normal.x = normal[0];
			m_normal.y = normal[1];
			m_normal.z = normal[2];
			
			if (colors)
			{
				int color_index = get_layer_element_index(colors->GetMappingMode(), colors->GetReferenceMode(), colors->GetIndexArray(), vertex_position, vertex_counter);
				FbxColor color = colors->GetDirectArray()[color_index];
				glm::vec4& m_color = mesh->vertex_colors[vertex_position];
				m_color.r = color.mRed;
				m_color.g = color.mGreen;
				m_color.b = color.mBlue;
				m_color.a = color.mAlpha;
			}
			
			if (uvs)
			{
				int poly_vertex_index = fbxmesh->GetTextureUVIndex(poly, v);
				int uv_index = -1;
				
				if (uvs->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					FbxLayerElement::EReferenceMode uv_ref_mode = uvs->GetReferenceMode();
					if (uv_ref_mode != FbxGeometryElement::eDirect)
					{
						uv_index = uvs->GetIndexArray().GetAt(vertex_counter);
					}
					else
					{
						uv_index = vertex_counter;
					}
				}
				else if (uvs->GetMappingMode() == FbxGeometryElement::eByControlPoint)
				{
					// TODO: support this
					assert(0);
				}
				
//				LOGV("uv_index: %i\n", uv_index);
				FbxVector2 uv = uvs->GetDirectArray().GetAt(uv_index);
				
				glm::vec2& m_uv = mesh->uvs[0][vertex_position];
				m_uv.x = uv[0];
				m_uv.y = uv[1];
			}
			
			++vertex_counter;
		}
	}
	
	// polygon count returns triangles if geometry converter was used to
	// triangulate the scene.
	LOGV("%striangles count: %i\n", state.indent(), fbxmesh->GetPolygonCount());
}

static std::string type_from_node(FbxNode* node)
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

static void populate_hierarchy(IndentState& state, datamodel::SceneNode* root, FbxNode* node)
{
	datamodel::SceneNode* scene_node = root;
	
	bool is_valid = true;
	
	state.push();
	
	// engine doesn't read lights or cameras
	if (node->GetLight() || node->GetCamera())
	{
		is_valid = false;
	}
	
	FbxNodeAttribute* node_attribute = node->GetNodeAttribute();
	if (node_attribute)
	{
		bool is_geometry = (
							node_attribute->GetAttributeType() == FbxNodeAttribute::eMesh ||
							node_attribute->GetAttributeType() == FbxNodeAttribute::eNurbs ||
							node_attribute->GetAttributeType() == FbxNodeAttribute::ePatch
							);
		
		if (is_geometry)
		{
			FbxGeometry* geometry = (FbxGeometry*)node_attribute;
			int blend_shape_count = geometry->GetDeformerCount(FbxDeformer::eBlendShape);
			LOGV("blend_shapes = %i\n", blend_shape_count);
		}
	}
	
	
	if (is_valid)
	{
		// create a new node
		scene_node = CREATE(datamodel::SceneNode);
		root->add_child(scene_node);
		
		// copy data
		scene_node->type = type_from_node(node);
		if (scene_node->type == "mesh")
		{
			scene_node->mesh = CREATE(datamodel::Mesh);
			// populate mesh from fbxnode
			state.push();
			load_mesh(state, node, scene_node->mesh);
			state.pop();
		}
		scene_node->name = node->GetName();
		FbxDouble3 translation = node->LclTranslation.Get();
		FbxDouble3 rotation = node->LclRotation.Get();
		FbxDouble3 scaling = node->LclScaling.Get();
		
		
		//			LOGV("%s\"%s\", (LCL) t = (%g %g %g), r = (%g %g %g), s = (%g %g %g)\n",
		//				 state.indent(),
		//				 scene_node->name.c_str(),
		//				 translation[0], translation[1], translation[2],
		//				 rotation[0], rotation[1], rotation[2],
		//				 scaling[0], scaling[1], scaling[2]
		//				 );
		
		//			{
		//				FbxVector4 translation = node->GetGeometricTranslation(FbxNode::eSourcePivot);
		//				FbxVector4 rotation = node->GetGeometricRotation(FbxNode::eSourcePivot);
		//				FbxVector4 scaling = node->GetGeometricScaling(FbxNode::eSourcePivot);
		//
		//				LOGV("%s\"%s\", (GEO) t = (%g %g %g), r = (%g %g %g), s = (%g %g %g)\n",
		//					 state.indent(),
		//					 scene_node->name.c_str(),
		//					 translation[0], translation[1], translation[2],
		//					 rotation[0], rotation[1], rotation[2],
		//					 scaling[0], scaling[1], scaling[2]
		//					 );
		//			}
		//			FbxAMatrix local_transform = node->EvaluateLocalTransform();
		
		FbxAMatrix global_transform = node->EvaluateGlobalTransform();
		to_mat4(global_transform, scene_node->global_transform);
		
		//			FbxVector4 scaling = node->EvaluateLocalScaling();
		//			FbxVector4 rotation = node->EvaluateLocalRotation();
		//			FbxVector4 translation = node->EvaluateLocalTranslation();
		
		
		scene_node->scale = glm::vec3(scaling[0], scaling[1], scaling[2]);
		// convert these to radians
		rotation[0] = mathlib::degrees_to_radians(rotation[0]);
		rotation[1] = mathlib::degrees_to_radians(rotation[1]);
		rotation[2] = mathlib::degrees_to_radians(rotation[2]);
		scene_node->rotation = glm::quat(glm::vec3(rotation[0], rotation[1], rotation[2]));
		scene_node->translation = glm::vec3(translation[0], translation[1], translation[2]);
		
		for (size_t index = 0; index < node->GetChildCount(); ++index)
		{
			populate_hierarchy(state, scene_node, node->GetChild(index));
		}
	}
	
	state.pop();
}


void AutodeskFbxReader::read(datamodel::SceneNode* root, util::DataStream& data_source)
{
	LOGV("TODO: switch this over to FbxStream\n");
	//		http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html
	//		http://www.gamedev.net/topic/653502-useful-things-you-might-want-to-know-about-fbxsdk/?hl=%2Bfbx+%2Bsdk#entry5149612
	//		http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/how-to-work-with-fbx-sdk-r3582
	//		http://forums.autodesk.com/t5/fbx-sdk/how-to-get-uv-coord-per-vertex/td-p/4239532
	// 		http://forums.autodesk.com/t5/fbx-sdk/splitmeshespermaterial-reached-limits/td-p/4239623
	const char* path = (const char*)data_source.get_data();
	
	LOGV("loading model: %s\n", path);
	
	FbxManager* m = FbxManager::Create();
	FbxIOSettings* settings = FbxIOSettings::Create(m, IOSROOT);
	
	FbxImporter* importer = FbxImporter::Create(m, "");
	
	if (!importer->Initialize(path, -1, m->GetIOSettings()))
	{
		LOGE("initialize exporter failed\n");
		LOGE("%s\n", importer->GetStatus().GetErrorString());
		return;
	}
	
	FbxScene* scene = FbxScene::Create(m, "test_scene");
	
	importer->Import(scene);
	
	FbxGeometryConverter converter(m);
	converter.Triangulate(scene, true);
	//converter.SplitMeshesPerMaterial(scene, false);
	
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
	
	int total_animation_stacks = scene->GetSrcObjectCount<FbxAnimStack>();
	LOGV("animation stacks: %i\n", total_animation_stacks);
	
	
	//	int total_bones = scene->GetPoseCount();
	int total_poses = scene->GetPoseCount();
	LOGV("pose count: %i\n", total_poses);
	
	FbxNode* fbxroot = scene->GetRootNode();
	if (fbxroot)
	{
		IndentState state;
		
		populate_hierarchy(state, root, fbxroot);
	}
	
	m->Destroy();
}

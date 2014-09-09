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


static void parse_materials(IndentState& state, FbxNode* node)
{
	// materials for this piece of geometry
	int total_materials = node->GetMaterialCount();
	LOGV("%stotal_materials: %i\n", state.indent(), total_materials);
	
	if (total_materials > 0)
	{
		for (int material_id = 0; material_id < total_materials; ++material_id)
		{
			FbxSurfaceMaterial* material = node->GetMaterial(material_id);
			LOGV("%smaterial: %s\n", state.indent(), material->GetName());
			
			
			if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
			{
				LOGV("%sfound lambert material!\n", state.indent());
			}
			else if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
			{
				LOGV("%sfound phong material!\n", state.indent());
			}
		}
	}
}

static void load_mesh(IndentState& state, FbxNode* node, datamodel::Mesh* mesh)
{
	FbxMesh* fbxmesh = node->GetMesh();
	
	parse_materials(state, node);
	
	fbxmesh->GenerateNormals();
	fbxmesh->GenerateTangentsData();
	
	int total_deformers = fbxmesh->GetDeformerCount();
//	LOGV("%stotal_deformers: %i\n", state.indent(), total_deformers);
	
	state.update();
	int total_triangles = fbxmesh->GetPolygonCount();
//	LOGV("%s# triangles: %i\n", state.indent(), total_triangles);
//	LOGV("%s# vertices: %i\n", state.indent(), fbxmesh->GetControlPointsCount());
	
	size_t total_indices = fbxmesh->GetPolygonCount()*3;
	
	mesh->indices.allocate(total_indices);
	mesh->vertices.allocate(total_indices);
	mesh->normals.allocate(total_indices);
//	LOGV("%s# indices: %i\n", state.indent(), total_indices);
	
	datamodel::Vertex* vertices = CREATE_ARRAY(datamodel::Vertex, total_indices);
	size_t vertex_index = 0;
	size_t local_index = 0;
	
	// uv coordinates
	FbxStringList uv_set_list;
	fbxmesh->GetUVSetNames(uv_set_list);
//	LOGV("%stotal_uv_sets: %i\n", state.indent(), uv_set_list.GetCount());
	
	
	// clamp the # of maximum uv sets
	int total_uv_sets = uv_set_list.GetCount();
	if (total_uv_sets > datamodel::MAX_SUPPORTED_UV_CHANNELS)
	{
		LOGW("%stotal uv sets exceeds MAX_SUPPORTED_UV_CHANNELS (%i)\n", state.indent(), datamodel::MAX_SUPPORTED_UV_CHANNELS);

		// print out the sets being ignored
		state.push();
		for (int item_index = datamodel::MAX_SUPPORTED_UV_CHANNELS; item_index < total_uv_sets; ++item_index)
		{
			FbxStringListItem* item = uv_set_list.GetItemAt(item_index);
			LOGW("%signoring set: \"%s\"\n", state.indent(), item->mString.Buffer());
		}
		state.pop();
		total_uv_sets = datamodel::MAX_SUPPORTED_UV_CHANNELS;
	}
	
	mesh->uvs.allocate(total_uv_sets);
	for (int uvset = 0; uvset < total_uv_sets; ++uvset)
	{
		mesh->uvs[uvset].allocate(total_indices);
	}

	for (int triangle_index = 0; triangle_index < total_triangles; ++triangle_index)
	{
		for (int local_vertex_id = 0; local_vertex_id < 3; ++local_vertex_id)
		{
			int index = fbxmesh->GetPolygonVertex(triangle_index, local_vertex_id);
//			LOGV("%sindex: %i\n", state.indent(), index);
			mesh->indices[local_index] = local_index;
			
			const FbxVector4& position = fbxmesh->GetControlPointAt(index);
//			LOGV("%svertex: %g %g %g\n", state.indent(), position[0], position[1], position[2]);
			
			FbxVector4 normal;
			fbxmesh->GetPolygonVertexNormal(triangle_index, local_vertex_id, normal);
//			LOGV("%snormal: %g %g %g\n", state.indent(), normal[0], normal[1], normal[2]);
			


			datamodel::Vertex* vertex = &vertices[vertex_index];
			vertex->position = glm::vec3(position[0], position[1], position[2]);
			vertex->normal = glm::vec3(normal[0], normal[1], normal[2]);
			for (int uvset = 0; uvset < total_uv_sets; ++uvset)
			{
				FbxVector2 uv;
				bool unmapped;
				FbxStringListItem* item = uv_set_list.GetItemAt(uvset);
				bool has_uv = fbxmesh->GetPolygonVertexUV(triangle_index, local_vertex_id, item->mString, uv, unmapped);
				if (!has_uv)
				{
					LOGE("%sUnable to get polygon vertex at [t=%i, v=%i, set=%s\n", state.indent(), triangle_index, local_vertex_id, item->mString.Buffer());
				}
	//			LOGV("%suv: %g %g\n", state.indent(), uv[0], uv[1]);
				vertex->uv[uvset] = glm::vec2(uv[0], uv[1]);
				mesh->uvs[uvset][vertex_index] = vertex->uv[uvset];
			}
			
			mesh->vertices[vertex_index] = vertex->position;
			mesh->normals[vertex_index] = vertex->normal;
			

			++vertex_index;
			++local_index;
		}
	}

//	LOGV("%svertex_index: %i\n", state.indent(), vertex_index);
	DESTROY_ARRAY(Vertex, vertices, mesh->indices.size());
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
//			LOGV("%sblend_shapes = %i\n", state.indent(), blend_shape_count);
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
		
		scene_node->scale = glm::vec3(scaling[0], scaling[1], scaling[2]);
		// convert these to radians
		rotation[0] = mathlib::degrees_to_radians(rotation[0]);
		rotation[1] = mathlib::degrees_to_radians(rotation[1]);
		rotation[2] = mathlib::degrees_to_radians(rotation[2]);
		scene_node->rotation = glm::quat(glm::vec3(rotation[0], rotation[1], rotation[2]));
		scene_node->translation = glm::vec3(translation[0], translation[1], translation[2]);
//		LOGV("translation: %g %g %g\n", scene_node->translation.x, scene_node->translation.y, scene_node->translation.z);
		
		for (size_t index = 0; index < node->GetChildCount(); ++index)
		{
			populate_hierarchy(state, scene_node, node->GetChild(index));
		}
	}
	
	state.pop();
}

namespace internal
{
	static FbxManager* _manager;
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
	
	FbxManager* manager = internal::_manager;
	
	FbxIOSettings* settings = FbxIOSettings::Create(manager, IOSROOT);
	FbxImporter* importer = FbxImporter::Create(manager, "");
	
	if (!importer->Initialize(path, -1, manager->GetIOSettings()))
	{
		LOGE("initialize exporter failed\n");
		LOGE("%s\n", importer->GetStatus().GetErrorString());
		return;
	}
	
	// query and print out the detected file version
	int file_version_major;
	int file_version_minor;
	int file_version_patch;
	importer->GetFileVersion(file_version_major, file_version_minor, file_version_patch);
	LOGV("FBX File Format: %i.%i.%i\n", file_version_major, file_version_minor, file_version_patch);
	
	FbxScene* scene = FbxScene::Create(manager, "test_scene");
	
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
	FbxSystemUnit system_unit = scene->GetGlobalSettings().GetSystemUnit();

	if (system_unit != FbxSystemUnit::m)
	{
		LOGV("Converting scene units from: %s\n", system_unit.GetScaleFactorAsString().Buffer());
		FbxSystemUnit::m.ConvertScene(scene);
	}
	
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
}

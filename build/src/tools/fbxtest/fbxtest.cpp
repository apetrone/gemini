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

#include <string>

#include <stdio.h>
#include <stdlib.h>

#include <gemini/mem.h>
#include <gemini/core.h>
#include <gemini/core/filesystem.h>
#include <gemini/core/log.h>
#include <gemini/core/xfile.h>
#include <gemini/util/stackstring.h>
#include <gemini/util/fixedarray.h>
#include <gemini/util/arg.h>
#include <gemini/util/datastream.h>

#include <gemini/mathlib.h>

#include <slim/xlog.h>

#include <json/json.h>

#include <fbxsdk.h>
#include <fbxsdk/utils/fbxgeometryconverter.h>
// need to support:
// static (non-animated) meshes
// hierarchical/skeletal meshes
// morph targets

// skeleton (bones in a hierarchical tree)
// mesh/geometry (vertices, normals, uvs[2], vertex-colors)


//https://docs.unrealengine.com/latest/INT/Engine/Content/FBX/index.html

struct IndentState
{
	size_t depth;
	std::string buffer;
	
	IndentState() : depth(0)
	{}
	
	void push()
	{
		++depth;
		update();
	}
	
	void pop()
	{
		--depth;
		update();
	}
	
	void update()
	{
		buffer.clear();
		for (size_t i = 0; i < depth; ++i)
		{
			buffer += "\t";
		}
	}
	
	const char* indent()
	{
		return buffer.c_str();
	}
};


#include <vector>
namespace datamodel
{
	const int MAX_SUPPORTED_UV_CHANNELS = 2;

	struct Mesh
	{
		FixedArray<glm::vec4> blend_indices;
		FixedArray<glm::vec4> blend_weights;
		FixedArray<glm::vec3> vertices;
		FixedArray<glm::vec3> normals;
		FixedArray<glm::vec4> vertex_colors;
		FixedArray<glm::vec2> uvs[MAX_SUPPORTED_UV_CHANNELS];
		uint8_t total_uv_sets;
		FixedArray<uint32_t> indices;
	};
	
	struct Skeleton
	{
		
	};

	typedef std::vector<struct SceneNode*, GeminiAllocator<struct SceneNode*>> SceneNodeVector;
	
	struct SceneNode
	{
		std::string name;
		std::string type;
		
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		
		SceneNode* parent;
		SceneNodeVector children;
		
		
		Mesh* mesh;
		Skeleton* skeleton;
		
		SceneNode()
		{
			parent = nullptr;
			mesh = nullptr;
			skeleton = nullptr;
		}
		
		virtual ~SceneNode()
		{
			SceneNodeVector::iterator it = children.begin();
			for( ; it != children.end(); ++it)
			{
				SceneNode* node = (*it);
				DESTROY(SceneNode, node);
			}
			
			children.clear();
			
			if (mesh)
			{
				DESTROY(Mesh, mesh);
			}
			
			if (skeleton)
			{
				DESTROY(Skeleton, skeleton);
			}
		}
		
		void add_child(SceneNode* child)
		{
			if (child->parent)
			{
				child->parent->remove_child(child);
			}
			
			child->parent = this;
			children.push_back(child);
		}
		
		void remove_child(SceneNode* child)
		{
			// find the child and detach from the old parent
			for (SceneNodeVector::iterator it = children.begin(); it != children.end(); ++it)
			{
				if (child == (*it))
				{
					children.erase(it);
					break;
				}
			}
		}
	};
}

#include <map>
#include <string>


namespace tools
{
	struct Result
	{
		const char* result_message;
		bool success;
		
		Result(bool result, const char* message = 0) : success(result), result_message(message) {}
		
		bool failed() const { return (success == false); }
		const char* message() const { return result_message; }
	};
}


namespace tools
{
	template <class Type>
	class Reader
	{

	public:
		Reader() {}
		virtual ~Reader() {}
		
		virtual void read(Type* model, util::DataStream& data) = 0;
	};
	
	template <class Type>
	class Writer
	{
	public:
		Writer() {}
		virtual ~Writer() {}
		
		virtual void write(Type* model, util::DataStream& data) = 0;
	};
	
	template <class Type>
	struct ArchiverExtension
	{
		Reader<Type>* reader;
		Writer<Type>* writer;

		ArchiverExtension() : reader(nullptr), writer(nullptr) {}
	};
	
	template <class Type>
	struct ExtensionRegistry
	{
		typedef std::map<std::string, ArchiverExtension<Type> > ExtensionMap;

		static ExtensionMap extensions;
	};
	
	template <class Type>
	typename ExtensionRegistry<Type>::ExtensionMap ExtensionRegistry<Type>::extensions;
	
	
	template <class Type>
	void register_extension(const std::string& extension, const ArchiverExtension<Type> & ptr)
	{
		ExtensionRegistry<Type>::extensions.insert(std::pair<std::string, ArchiverExtension<Type> >(extension, ptr));
	}
	
	template <class Type>
	void purge_registry()
	{
		for (auto data : ExtensionRegistry<Type>::extensions)
		{
			ArchiverExtension<Type>& ext = data.second;
			if (ext.reader)
			{
				DESTROY(Reader<Type>, ext.reader);
			}
			
			if (ext.writer)
			{
				DESTROY(Writer<Type>, ext.writer);
			}
		}
	}


	template <class Type>
	const ArchiverExtension<Type> find_archiver_for_extension(const std::string& target_extension, uint8_t flags = 0)
	{
		for (auto v : ExtensionRegistry<Type>::extensions)
		{
			const ArchiverExtension<Type>& archiver_extension = v.second;
			
			if (target_extension == v.first)
			{
				if (flags == 0)
				{
					return archiver_extension;
				}
//				
//				if ((flags & 1) && archiver_extension.reader)
//				{
//					return archiver_extension;
//				}
//				else if ((flags & 2) && archiver_extension.writer)
//				{
//					return archiver_extension;
//				}
			}
		}
	
		return ArchiverExtension<Type>();
	}



	Result convert_scene(const char* input_path, const char* output_path)
	{
		datamodel::SceneNode root;
		
		// TODO: get extension from input_path
		std::string ext = "fbx";
		
		// verify we can read the format
		const ArchiverExtension<datamodel::SceneNode> archiver_extension = find_archiver_for_extension<datamodel::SceneNode>(ext);
		tools::Reader<datamodel::SceneNode>* reader = archiver_extension.reader;
		if (!reader)
		{
			LOGE("no reader found for extension: %s\n", ext.c_str());
			return Result(false, "Unable to read format");
		}
		
		// verify we can write the format
		ext = "model";
		const ArchiverExtension<datamodel::SceneNode> writer_extension = find_archiver_for_extension<datamodel::SceneNode>(ext);
		tools::Writer<datamodel::SceneNode>* writer = writer_extension.writer;
		if (!writer)
		{
			LOGE("no writer found for extension: %s\n", ext.c_str());
			return Result(false, "Unable to write format");
		}
		
		util::MemoryStream mds;
		mds.data = (uint8_t*)input_path;
		reader->read(&root, mds);
		
		util::ResizableMemoryStream rs;
		writer->write(&root, rs);

		rs.rewind();
		
		xfile_t out = xfile_open(output_path, XF_WRITE);
		if (xfile_isopen(out))
		{
			xfile_write(out, rs.get_data(), rs.get_data_size(), 1);
			xfile_close(out);
		}
		return Result(true);
	}
}

using namespace tools;

class AutodeskFbxReader : public tools::Reader<datamodel::SceneNode>
{
public:

	AutodeskFbxReader() : tools::Reader<datamodel::SceneNode>() {}
	virtual ~AutodeskFbxReader() {}
	
	
	void read_vertices(IndentState& state, FbxMesh* fbxmesh, datamodel::Mesh* mesh)
	{
		//
		// copy vertices
		int total_vertices = fbxmesh->GetControlPointsCount();
//		LOGV("%stotal vertices: %i\n", state.indent(), total_vertices);
		mesh->vertices.allocate(total_vertices);
		for (int vertex_id = 0; vertex_id < total_vertices; ++vertex_id)
		{
			const FbxVector4& vertex = fbxmesh->GetControlPointAt(vertex_id);
			glm::vec3& mesh_vertex = mesh->vertices[vertex_id];
			mesh_vertex.x = vertex[0];
			mesh_vertex.y = vertex[1];
			mesh_vertex.z = vertex[2];
		}
	}
	
	void load_mesh(IndentState& state, FbxNode* node, datamodel::Mesh* mesh)
	{
		FbxMesh* fbxmesh = node->GetMesh();

		int total_layers = fbxmesh->GetLayerCount();
		LOGV("total layers: %i\n", total_layers);
		assert(total_layers == 1);



		int bad_polys_removed = fbxmesh->RemoveBadPolygons();
		if (bad_polys_removed > 0)
		{
			LOGV("bad_polys_removed = %i\n", bad_polys_removed);
		}

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
		
		fbxmesh->GenerateTangentsData();
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
			mesh->vertex_colors.allocate(total_vertices);
		}
		
		if (uvs)
		{
			mesh->uvs[0].allocate(total_vertices);
		}

		int total_polygons = fbxmesh->GetPolygonCount();
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
//				LOGV("v: %i [%g %g %g]\n", v, vertex[0], vertex[1], vertex[2]);
				
				FbxVector4 normal = normals->GetDirectArray()[vertex_position];
				glm::vec3& m_normal = mesh->normals[vertex_position];
				m_normal.x = normal[0];
				m_normal.y = normal[1];
				m_normal.z = normal[2];
//				LOGV("n: %i [%g %g %g]\n", v, normal[0], normal[1], normal[2]);

				if (colors)
				{
					FbxColor color = colors->GetDirectArray()[vertex_position];
//					LOGV("c: %i %g %g %g %g\n", v, color.mRed, color.mGreen, color.mBlue, color.mAlpha);
					glm::vec4& m_color = mesh->vertex_colors[vertex_position];
					m_color.r = color.mRed;
					m_color.g = color.mGreen;
					m_color.b = color.mBlue;
					m_color.a = color.mAlpha;
				}
				
				if (uvs)
				{
					int mesh_index = fbxmesh->GetTextureUVIndex(poly, v);
					FbxVector2 uv = uvs->GetDirectArray()[mesh_index];
//					LOGV("mesh_index: %i [%g %g]\n", uv[0], uv[1]);
					glm::vec2& m_uv = mesh->uvs[0][vertex_position];
					m_uv.x = uv[0];
					m_uv.y = uv[1];
				}
			}
		}
		
		// polygon count returns triangles if geometry converter was used to
		// triangulate the scene.
		LOGV("%striangles count: %i\n", state.indent(), fbxmesh->GetPolygonCount());
	}
	
	void print_node(IndentState& state, FbxNode* node)
	{
		const char* node_name = node->GetName();
		
		FbxDouble3 translation = node->LclTranslation.Get();
		FbxDouble3 rotation = node->LclRotation.Get();
		FbxDouble3 scaling = node->LclScaling.Get();
		
		state.push();
		
		if (node->GetSkeleton())
		{
			LOGV("%sfound skeleton\n", state.indent());
		}
		else if (node->GetMesh())
		{
			LOGV("%sfound mesh\n", state.indent());
		}
		else if (node->GetCamera())
		{
			LOGV("%sfound camera\n", state.indent());
		}
		else if (node->GetLight())
		{
			LOGV("%sfound light\n", state.indent());
		}
		
		LOGV("%s\"%s\", t = (%g %g %g), r = (%g %g %g), s = (%g %g %g)\n",
			 state.indent(),
			 node_name,
			 translation[0], translation[1], translation[2],
			 rotation[0], rotation[1], rotation[2],
			 scaling[0], scaling[1], scaling[2]
			 );
		
		for (size_t index = 0; index < node->GetChildCount(); ++index)
		{
			print_node(state, node->GetChild(index));
		}
		
		state.pop();
	}
	
	std::string type_from_node(FbxNode* node)
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
	
	void populate_hierarchy(IndentState& state, datamodel::SceneNode* root, FbxNode* node)
	{
		datamodel::SceneNode* scene_node = root;
		
		bool is_valid = true;
		
		state.push();

		// engine doesn't read lights or cameras
		if (node->GetLight() || node->GetCamera())
		{
			is_valid = false;
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
			
//			FbxAMatrix local_transform = node->EvaluateLocalTransform();
			
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
	
	virtual void read(datamodel::SceneNode* root, util::DataStream& data_source)
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
		
		
		int total_animation_stacks = scene->GetSrcObjectCount<FbxAnimStack>();
		LOGV("animation stacks: %i\n", total_animation_stacks);
		
		
		//	int total_bones = scene->GetPoseCount();
		int total_poses = scene->GetPoseCount();
		LOGV("pose count: %i\n", total_poses);
		
		FbxNode* fbxroot = scene->GetRootNode();
		if (fbxroot)
		{
			IndentState state;
			
			// traverse
			//print_node(state, fbxroot);
			
			// add children
			for (size_t index = 0; index < fbxroot->GetChildCount(); ++index)
			{
				populate_hierarchy(state, root, fbxroot->GetChild(index));
			}
		}
		
		
		m->Destroy();
	}
};


class JsonSceneWriter : public tools::Writer<datamodel::SceneNode>
{
public:

	void append_node(datamodel::SceneNode* node, Json::Value& jnodes)
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
			for (size_t uv_set_id = 0; uv_set_id < node->mesh->total_uv_sets; ++uv_set_id)
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


	virtual void write(datamodel::SceneNode* root, util::DataStream& source)
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
};

void register_types()
{
	ArchiverExtension<datamodel::SceneNode> ext;
	ext.reader = CREATE(AutodeskFbxReader);
	register_extension<datamodel::SceneNode>("fbx", ext);
	
	ext.reader = 0;
	ext.writer = CREATE(JsonSceneWriter);
	register_extension<datamodel::SceneNode>("model", ext);
}

int main(int argc, char** argv)
{
	memory::startup();
	core::startup();
	
//	args::Argument* asset_root = args::add("asset_root","-d", "--asset-root", 0, 0);
	args::Argument* input_file = args::add("input_file", "-f", "--input", 0, 0);
	args::Argument* output_file = args::add("output_file", "-o", "--output", 0, 0);
//	args::Argument* output_root = args::add("output_root", "-o", "--output-root", 0, 0);
//	args::Argument* convert_axis = args::add("convert_zup_to_yup", "-y", 0, args::NO_PARAMS | args::NOT_REQUIRED, 0);
	
	if (!args::parse_args(argc, argv))
	{
		LOGE("Argument parsing failed.\n");
		return -1;
	}
	
	register_types();

	Result result = tools::convert_scene(input_file->string, output_file->string);
	if (result.failed())
	{
		LOGV("conversion failed: %s\n", result.message());
	}
	
	purge_registry<datamodel::SceneNode>();

	core::shutdown();
	memory::shutdown();
	return 0;
}
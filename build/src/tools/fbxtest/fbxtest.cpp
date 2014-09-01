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

#include <gemini/mathlib.h>

#include <slim/xlog.h>

#include <fbxsdk.h>

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
		FixedArray<uint32_t> indices;
	};

	struct SceneNode
	{
		glm::mat4 local_to_world;
	};
	
	
	
}

#include <map>
#include <string>

namespace tools
{
	class DataSource
	{
	public:
		virtual ~DataSource() {}
		
		virtual uint8_t* get_data() const = 0;
	};
	
	class MemoryDataSource : public DataSource
	{
	public:
		uint8_t* data;
		size_t data_length;
		
		virtual uint8_t* get_data() const { return data; }
	};

	template <class Type>
	class Reader
	{

	public:
		Reader() {}
		virtual ~Reader() {}
		
		virtual void read(Type* model, DataSource& data) = 0;
	};
	
	
	
	template <class Type>
	struct ArchiverExtension
	{
		Reader<Type>* reader;
//		Writer<Type>* writer;

		ArchiverExtension() : reader(nullptr) {}
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
	const ArchiverExtension<Type> find_archiver_for_extension(const std::string& target_extension, uint8_t flags = 1)
	{
		for (auto v : ExtensionRegistry<Type>::extensions)
		{
			LOGV("ext: %s\n", v.first.c_str());
			const ArchiverExtension<Type>& archiver_extension = v.second;
			
			if (target_extension == v.first)
			{
				if (flags == 0)
				{
					return archiver_extension;
				}
				
				if ((flags & 1) && archiver_extension.reader)
				{
					return archiver_extension;
				}
//				else if ((flags && 2) && archiver_extension.writer)
//				{
//					return archiver_extension;
//				}
			}
		}
	
		return ArchiverExtension<Type>();
	}
	

	void convert_scene(const char* input_path, const char* output_path)
	{
		datamodel::SceneNode root;
		
		// TODO: get extension from input_path
		std::string ext = "fbx";
		
		const ArchiverExtension<datamodel::SceneNode> archiver_extension = find_archiver_for_extension<datamodel::SceneNode>(ext);
		tools::Reader<datamodel::SceneNode>* reader = archiver_extension.reader;
		if (!reader)
		{
			LOGE("no reader found for extension: %s\n", ext.c_str());
			return;
		}
		
		MemoryDataSource mds;
		mds.data = (uint8_t*)input_path;
		
		// TODO: until I switch the reader to use streams, simply pass input_path
		reader->read(&root, mds);
	}
}

using namespace tools;

class AutodeskFbxReader : public tools::Reader<datamodel::SceneNode>
{
public:

	AutodeskFbxReader() : tools::Reader<datamodel::SceneNode>() {}
	virtual ~AutodeskFbxReader() {}
	
	void load_mesh(IndentState& state, FbxNode* node)
	{
		FbxMesh* mesh = node->GetMesh();
		int total_vertices = mesh->GetControlPointsCount();
		LOGV("%stotal vertices: %i\n", state.indent(), total_vertices);
		//	FbxVector4 v = mesh->GetControlPointAt(0);
		
		int total_indices = mesh->GetPolygonVertexCount();
		LOGV("%stotal indices: %i\n", state.indent(), total_indices);
		//	int* indices = mesh->GetPolygonVertices();
		
		FbxGeometryElementNormal* normal_element = mesh->GetElementNormal();
		if (normal_element)
		{
			int total_normals = mesh->GetPolygonCount() * 3;
			LOGV("%stotal normals: %i\n", state.indent(), total_normals);
		}
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
			state.push();
			load_mesh(state, node);
			state.pop();
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
	
	virtual void read(datamodel::SceneNode* root, DataSource& data_source)
	{
		LOGV("TODO: switch this over to FbxStream\n");
//		http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html
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
			print_node(state, fbxroot);
		}
		
		
		m->Destroy();
	}
};


void register_types()
{
	// TODO: load scene node archivers
	ArchiverExtension<datamodel::SceneNode> ext;
	ext.reader = new AutodeskFbxReader();

	register_extension<datamodel::SceneNode>("fbx", ext);
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

	tools::convert_scene(input_file->string, output_file->string);

	core::shutdown();
	memory::shutdown();
	return 0;
}
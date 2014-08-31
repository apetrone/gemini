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

#include <slim/xlog.h>

#include <fbxsdk.h>

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

void test(const char* path)
{
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
	
	FbxNode* root = scene->GetRootNode();
	if (root)
	{
		IndentState state;
		
		// traverse
		print_node(state, root);
	}
	
	

	
	m->Destroy();
}


int main(int argc, char** argv)
{
	memory::startup();
	core::startup();
	
	test(argv[1]);

	core::shutdown();
	memory::shutdown();
	return 0;
}
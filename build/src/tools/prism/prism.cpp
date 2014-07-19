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
#include <gemini/core.h>
#include <gemini/core/filesystem.h>


#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <slim/xlog.h>

#include <gemini/core/log.h>

#include <gemini/util/stackstring.h>
#include <gemini/core/xfile.h>

#include <gemini/util/fixedarray.h>
#include <gemini/util/arg.h>

#include <json/json.h>

#include "meshdata.h"

using namespace prism;

const char TOOL_NAME[] = "prism";
const char TOOL_VERSION[] = "alpha 1.0.0";

struct ToolEnvironment
{
	bool convert_zup_to_yup;
	
	ToolEnvironment()
	{
		convert_zup_to_yup = false;
	}
	
	void print_settings()
	{
		LOGV("convert_zup_to_yup: %s\n", convert_zup_to_yup ? "true": "false");
	}
};






void convert_and_write_model(ToolEnvironment& env, const aiScene* scene, const char* output_path)
{
	prism::MeshData modeldata;
	
	Json::Value root;

	const aiMesh *mesh = 0;

	
	LOGV("model has %i meshes\n", scene->mNumMeshes);
	LOGV("model has %i animations\n", scene->mNumAnimations);
	LOGV("model has %i materials\n", scene->mNumMaterials);
	LOGV("model has %i textures\n", scene->mNumTextures);
	LOGV("model has %i cameras\n", scene->mNumCameras);
	
	root["name"] = "model";
	root["info"] = "converted by prism";
	
	Json::Value jinfo(Json::objectValue);
	Json::Value jtransform(Json::objectValue);

	jinfo["tool"] = TOOL_NAME;
	jinfo["version"] = TOOL_VERSION;
	
	Json::Value jmaterial_array(Json::arrayValue);
	Json::Value jgeometry_array(Json::arrayValue);
	
	Json::Value jbones_array(Json::arrayValue);
	
	
	Json::Value hierarchy;

	
	MeshData meshdata;
	
	
	// traverse all nodes in the scene
	traverse_nodes(meshdata, scene, hierarchy);
	
	// loop through all meshes
	for( size_t m = 0; m < scene->mNumMeshes; ++m )
	{
		mesh = scene->mMeshes[m];
		Json::Value jgeometry;
		
		Json::Value jvertices(Json::arrayValue);
		Json::Value jnormals(Json::arrayValue);
		Json::Value juvs(Json::arrayValue);
		Json::Value jfaces(Json::arrayValue);
		
		// any one of these is an error condition otherwise.
		assert( mesh->HasTextureCoords(0) );
		assert( mesh->HasNormals() );
		assert( mesh->HasTangentsAndBitangents() );

		jgeometry["name"] = mesh->mName.C_Str();
		LOGV("inspecting mesh: %i, \"%s\"\n", m, mesh->mName.C_Str());
		LOGV("\tvertices: %i\n", mesh->mNumVertices);
		LOGV("\tfaces: %i\n", mesh->mNumFaces);
		LOGV("\tbones: %i\n", mesh->mNumBones);
	
		meshdata.read_bones(mesh, jbones_array);
		
		
		if (mesh->HasNormals() && mesh->HasTextureCoords(0))
		{
			for(unsigned int vertex = 0; vertex < mesh->mNumVertices; ++vertex)
			{
				const aiVector3D & pos = mesh->mVertices[vertex];
				const aiVector3D & normal = mesh->mNormals[vertex];
				const aiVector3D & uv = mesh->mTextureCoords[0][vertex];
				//const aiVector3D & bitangent = mesh->mBitangents[vertex];
				//const aiVector3D & tangent = mesh->mTangents[vertex];
				
				// Need to transform the vectors from Z-Up to Y-Up.
				aiMatrix3x3 rot;
				
				if (env.convert_zup_to_yup)
				{
					aiMatrix3x3::Rotation((-M_PI_2), aiVector3D(1, 0, 0), rot);
				}
				
				const aiVector3D tr_pos = rot * pos;
				
				jvertices.append(tr_pos.x);
				jvertices.append(tr_pos.y);
				jvertices.append(tr_pos.z);
				
				const aiVector3D tr_normal = rot * normal;

				jnormals.append(tr_normal.x);
				jnormals.append(tr_normal.y);
				jnormals.append(tr_normal.z);
				
				juvs.append(uv.x);
				juvs.append(uv.y);
			}

			aiFace* face;
			for (uint32_t face_index = 0; face_index < mesh->mNumFaces; ++face_index)
			{
				face = &mesh->mFaces[face_index];
				jfaces.append(face->mIndices[0]);
				jfaces.append(face->mIndices[1]);
				jfaces.append(face->mIndices[2]);
			}

			jgeometry["positions"] = jvertices;
			jgeometry["normals"] = jnormals;
			jgeometry["uvs"] = juvs;
			jgeometry["indices"] = jfaces;
		}
		else
		{
			fprintf(stdout, "Mesh %zu is missing Normals\n", m);
		}
		
		
		
		
		// TODO: error checking here...
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		
		if (material->GetTextureCount(aiTextureType_AMBIENT) > 0) { LOGV("material has an ambient texture\n"); }
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) { LOGV("material has a diffuse texture\n"); }
		if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0) { LOGV("material has an emissive texture\n"); }
		if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) { LOGV("material has an specular texture\n"); }
		
		aiString texture_path;
		if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path, 0, 0, 0, 0, 0))
		{
			StackString<4096> texpath = texture_path.C_Str();

			// TODO: verify there's a material by this name in the input/output folder?

			// COLLADA does some weird shit with texture names.
			jgeometry["material_id"] = jmaterial_array.size();
			Json::Value jmaterial;
			jmaterial["name"] = texpath.basename().remove_extension()();
			jmaterial_array.append(jmaterial);
		}

		jgeometry_array.append(jgeometry);
	}
	
	root["geometry"] = jgeometry_array;
	root["materials"] = jmaterial_array;
	root["info"] = jinfo;
	root["transform"] = jtransform;
	
	
	
	LOGV("TODO: should normalize bone weights!\n");
	root["bones"] = jbones_array;
	
	Json::Value janimations(Json::arrayValue);

	Animation animation_data;

	const aiAnimation* animation = 0;
	for (size_t animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
	{
		Json::Value janimation;
		
		animation = scene->mAnimations[animation_index];

		LOGV("inspecting animation: %i, \"%s\"\n", animation_index, animation->mName.C_Str());
		meshdata.read_animation(animation_data, animation, janimation);
		
		janimations.append(janimation);
		

	}
	
	//fprintf(stdout, "Loaded %zu meshes ready for drawing\n", meshes.size());
	
	root["animations"] = janimations;
	
	meshdata.print_nodes();
	
	Json::StyledWriter writer;
	xfile_t out = xfile_open(output_path, XF_WRITE);
	if (xfile_isopen(out))
	{
		std::string buf = writer.write(root);
		xfile_write(out, buf.c_str(), buf.length(), 1);
		xfile_close(out);
	}
}

void test_load_scene(ToolEnvironment& env, const char* asset_root, const char* input_file, const char* output_path)
{
	Assimp::Importer importer;

	StackString<2048> input_filename = asset_root;
	input_filename.append("/").append(input_file);
	
	// Bone matrices may be incorrect with aiProcess_PreTransformVertices set.
	unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;

	importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
	const aiScene* scene = importer.ReadFile(input_filename(), flags);
	if (scene)
	{
		if ( scene->HasMeshes() )
		{
			StackString<2048> fullpath = output_path;
			fullpath.append("/");
			fullpath = fullpath.append(input_file).remove_extension().append(".model");
			LOGV("fullpath = %s\n", fullpath());
			convert_and_write_model(env, scene, fullpath());
		}
		
		importer.FreeScene();
	}
	else
	{
		LOGE("Unable to open model: %s, (%s)\n", input_filename(), importer.GetErrorString());
	}
}



int main(int argc, char** argv)
{
	memory::startup();
	core::startup();

	arg_t* asset_root = arg_add("asset_root", "-a", "--asset-root", 0, 0);
	arg_t* input_file = arg_add("input_file", "-f", "--input", 0, 0);
	arg_t* output_root = arg_add("output_root", "-o", "--output-root", 0, 0);
	arg_t* convert_axis = arg_add("convert_zup_to_yup", "-y", 0, ARG_NO_PARAMS | ARG_NOT_REQUIRED, 0);
	
	if (arg_parse(argc, argv) != 0)
	{
		return -1;
	}
	
	ToolEnvironment env;
	env.convert_zup_to_yup = convert_axis->integer;
	env.print_settings();
	
	//	test_function();
	test_load_scene(env, asset_root->string, input_file->string, output_root->string);

	core::shutdown();
	memory::shutdown();
	return 0;
}
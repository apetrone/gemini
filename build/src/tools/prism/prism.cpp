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

#include <gemini/util/arg.h>

void test_function()
{
	fprintf(stdout, "Hello, World!\n");
	

	
	size_t buffer_length = 0;
	void* buffer = core::filesystem::file_to_buffer("input.blend", 0, &buffer_length);
	
	// do something with buffer

	DEALLOC(buffer);
}

void process_scene(const aiScene* scene)
{
	const aiMesh *mesh = 0;
	
	
	LOGV("model has %i meshes\n", scene->mNumMeshes);
	LOGV("model has %i animations\n", scene->mNumAnimations);
	LOGV("model has %i materials\n", scene->mNumMaterials);
	LOGV("model has %i textures\n", scene->mNumTextures);
	LOGV("model has %i cameras\n", scene->mNumCameras);
		
	// loop through all meshes and
	for( size_t m = 0; m < scene->mNumMeshes; ++m )
	{
		mesh = scene->mMeshes[m];
		LOGV("inspecting mesh: %i, \"%s\"\n", m, mesh->mName.C_Str());
		LOGV("\tvertices: %i\n", mesh->mNumVertices);
		LOGV("\tfaces: %i\n", mesh->mNumFaces);
		LOGV("\tbones: %i\n", mesh->mNumBones);

		
		if (mesh->HasNormals())
		{
			//Geometry * geometry = new Geometry();
			//geometry->create_from_mesh(mesh);
			//meshes.push_back(geometry);
		}
		else
		{
			fprintf(stdout, "Mesh %zu is missing Normals\n", m);
		}
	}
	
	const aiAnimation* animation = 0;
	for (size_t index = 0; index < scene->mNumAnimations; ++index)
	{
		animation = scene->mAnimations[index];
		LOGV("inspecting animation: %i, \"%s\"\n", index, animation->mName.C_Str());
		LOGV("\tduration: %g\n", animation->mDuration);
		LOGV("\tticks_per_second: %g\n", animation->mTicksPerSecond);
		LOGV("\tbone channels: %i\n", animation->mNumChannels);
		LOGV("\tmesh channels: %i\n", animation->mNumMeshChannels);

		const aiNodeAnim* node = 0;
		for (size_t channel = 0; channel < animation->mNumChannels; ++channel)
		{
			node = animation->mChannels[channel];
			LOGV("\tinspecting bone/node %i \"%s\" ...\n", channel, node->mNodeName.C_Str());
			LOGV("\t\tPosition Keys: %i\n", node->mNumPositionKeys);
			LOGV("\t\tRotation Keys: %i\n", node->mNumRotationKeys);
			LOGV("\t\tScaling Keys: %i\n", node->mNumScalingKeys);
		}
		
		const aiMeshAnim* anim = 0;
		for (size_t channel = 0; channel < animation->mNumMeshChannels; ++channel)
		{
			anim = animation->mMeshChannels[channel];
		}
	}
	
	//fprintf(stdout, "Loaded %zu meshes ready for drawing\n", meshes.size());
}

void test_load_scene(const char* input_file, const char* output_path)
{
	Assimp::Importer importer;
	
	size_t model_size = 0;
	void* buffer = core::filesystem::file_to_buffer(input_file, 0, &model_size);
	char* buf = (char*)buffer;
	
	if (buf)
	{
		// Bone matrices may be incorrect with aiProcess_PreTransformVertices set.
		unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;

		StackString<1024> filepath;
		core::filesystem::absolute_path_from_relative(&filepath[0], input_file);
		
		const aiScene* scene = importer.ReadFile(filepath(), flags);
		if (scene)
		{
			if ( scene->HasMeshes() )
			{
				process_scene(scene);
			}
			
			importer.FreeScene();
		}
		else
		{

			LOGE("Unable to open model: %s, (%s)\n", input_file, importer.GetErrorString());
		}
		
		StackString<1024> fullpath = output_path;
		fullpath.append("/");
		fullpath = fullpath.append(filepath.basename()).remove_extension().append(".model");
		LOGV("fullpath = %s\n", fullpath());
#if 0
		xfile_t out = xfile_open(fullpath(), XF_WRITE);
		if (xfile_isopen(out))
		{
			xfile_write(out, "test", 4, 1);
			xfile_close(out);
		}
#endif
		
		DEALLOC(buffer);
	}
}


int main(int argc, char** argv)
{
	memory::startup();
	core::startup();



	arg_t* source_file = arg_add("source_file", "-s", "--source", 0, 0);
	arg_t* destination_path = arg_add("destination_path", "-o", "--output-dir", 0, 0);
	
	if (arg_parse(argc, argv) != 0)
	{
		return -1;
	}
	
	//	test_function();
	test_load_scene(source_file->string, destination_path->string);

	core::shutdown();
	memory::shutdown();
	return 0;
}
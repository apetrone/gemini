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
	
	// loop through all meshes and
	for( size_t m = 0; m < scene->mNumMeshes; ++m )
	{
		mesh = scene->mMeshes[m];
		fprintf(stdout, "processing mesh: %s\n", mesh->mName.C_Str());
		
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
	
	//fprintf(stdout, "Loaded %zu meshes ready for drawing\n", meshes.size());
}

void test_load_scene(const char* path)
{
	Assimp::Importer importer;
	
	size_t model_size = 0;
	void* buffer = core::filesystem::file_to_buffer(path, 0, &model_size);
	char* buf = (char*)buffer;
	
	if (buf)
	{
		unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;

		StackString<1024> filepath;
		core::filesystem::absolute_path_from_relative(&filepath[0], path);
		
		const aiScene* scene = importer.ReadFile(filepath(), flags);
		if (scene)
		{
			if ( scene->HasMeshes() )
			{
				fprintf(stdout, "Scene contains %i mesh(es)\n", scene->mNumMeshes);
				
				process_scene(scene);
			}
			
			importer.FreeScene();
		}
		else
		{
			LOGE("Unable to open model: %s\n", path);
		}
		
		StackString<1024> fullpath = filepath.dirname().append(filepath.basename()).remove_extension().append(".model");
			
		xfile_t out = xfile_open(fullpath(), XF_WRITE);
		if (xfile_isopen(out))
		{
			xfile_write(out, "test", 4, 1);
			xfile_close(out);
		}
		
		
		
		
		DEALLOC(buffer);
	}
}


int main(int argc, char** argv)
{
	memory::startup();
	core::startup();

	test_function();
	test_load_scene("models/test.blend");

	core::shutdown();
	memory::shutdown();
	return 0;
}
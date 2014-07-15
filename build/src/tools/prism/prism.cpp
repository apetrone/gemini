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

#include <json/json.h>

// show output at build time regarding glm
//#define GLM_MESSAGES 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
//#include <glm/gtc/noise.hpp>
//#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

void test_function()
{
	fprintf(stdout, "Hello, World!\n");
	

	
	size_t buffer_length = 0;
	void* buffer = core::filesystem::file_to_buffer("input.blend", 0, &buffer_length);
	
	// do something with buffer

	DEALLOC(buffer);
}

struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
	glm::vec4 tangent;
};

void convert_and_write_model(const aiScene* scene, const char* output_path)
{
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

	
	Json::Value jmaterial_array(Json::arrayValue);
	Json::Value jgeometry_array(Json::arrayValue);
	
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
	
		if (mesh->HasNormals() && mesh->HasTextureCoords(0))
		{
			for(unsigned int vertex = 0; vertex < mesh->mNumVertices; ++vertex)
			{
				const aiVector3D & pos = mesh->mVertices[vertex];
				const aiVector3D & normal = mesh->mNormals[vertex];
				const aiVector3D & uv = mesh->mTextureCoords[0][vertex];
				//const aiVector3D & bitangent = mesh->mBitangents[vertex];
				//const aiVector3D & tangent = mesh->mTangents[vertex];
				
				jvertices.append(pos.x);
				jvertices.append(pos.y);
				jvertices.append(pos.z);
				
				jnormals.append(normal.x);
				jnormals.append(normal.y);
				jnormals.append(normal.z);
				
				juvs.append(uv.x);
				juvs.append(uv.y);
				
				//v->position = glm::vec3(pos.x, pos.y, pos.z);
				//v->uv = glm::vec2(uv.x, uv.y);
				//v->normal = glm::normalize(glm::vec3(normal.x, normal.y, normal.z));
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
		
		
		if (mesh->mNumBones > 0)
		{
			LOGV("inspecting bones...\n");
			const aiBone* bone = 0;
			for (size_t boneid = 0; boneid < mesh->mNumBones; ++boneid)
			{
				bone = mesh->mBones[boneid];
				LOGV("\tbone %i, \"%s\"\n", boneid, bone->mName.C_Str());
				LOGV("\tweights: %i\n", bone->mNumWeights);
				
				aiMatrix4x4 offset = bone->mOffsetMatrix;
				for (size_t weight = 0; weight < bone->mNumWeights; ++weight)
				{
					aiVertexWeight* w = &bone->mWeights[weight];
					LOGV("\tweight (%i) [vertex: %i -> weight: %2.2f\n", weight, w->mVertexId, w->mWeight);
				}
				
			}
		}
		
		
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		jgeometry["material_id"] = jmaterial_array.size();
		
		Json::Value jmaterial;
		jmaterial["name"] = Json::valueToString(mesh->mMaterialIndex);
		jmaterial_array.append(jmaterial);
		
		jgeometry_array.append(jgeometry);
	}
	root["geometry"] = jgeometry_array;
	root["materials"] = jmaterial_array;
	root["info"] = jinfo;
	root["transform"] = jtransform;
	
	const aiAnimation* animation = 0;
	for (size_t index = 0; index < scene->mNumAnimations; ++index)
	{
		animation = scene->mAnimations[index];
		LOGV("inspecting animation: %i, \"%s\"\n", index, animation->mName.C_Str());
		LOGV("\tduration: %g\n", animation->mDuration);
		LOGV("\tticks_per_second: %g\n", animation->mTicksPerSecond);
		LOGV("\tbone channels (skeletal): %i\n", animation->mNumChannels);
		LOGV("\tmesh channels (vertex): %i\n", animation->mNumMeshChannels);

		// bone/node-based animation
		const aiNodeAnim* node = 0;
		for (size_t channel = 0; channel < animation->mNumChannels; ++channel)
		{
			node = animation->mChannels[channel];
			LOGV("\tinspecting bone/node %i \"%s\" ...\n", channel, node->mNodeName.C_Str());
			LOGV("\t\tPosition Keys: %i\n", node->mNumPositionKeys);
			for (size_t key = 0; key < node->mNumPositionKeys; ++key)
			{
				const aiVectorKey* vkey = &node->mPositionKeys[key];
				LOGV("\t\t\tT @ %2.2f -> %2.2f %2.2f %2.2f\n", vkey->mTime, vkey->mValue.x, vkey->mValue.y, vkey->mValue.z);
			}
			
			LOGV("\t\tRotation Keys: %i\n", node->mNumRotationKeys);
			for (size_t key = 0; key < node->mNumRotationKeys; ++key)
			{
				const aiQuatKey* qkey = &node->mRotationKeys[key];
				LOGV("\t\t\tR @ %2.2f -> %2.2f %2.2f %2.2f %2.2f\n", qkey->mTime, qkey->mValue.x, qkey->mValue.y, qkey->mValue.z, qkey->mValue.w);
			}
			
			LOGV("\t\tScaling Keys: %i\n", node->mNumScalingKeys);
			for (size_t key = 0; key < node->mNumScalingKeys; ++key)
			{
				const aiVectorKey* vkey = &node->mScalingKeys[key];
				LOGV("\t\t\tS @ %2.2f -> %2.2f %2.2f %2.2f\n", vkey->mTime, vkey->mValue.x, vkey->mValue.y, vkey->mValue.z);
			}
		}
		
		
		// vertex-based animation
		const aiMeshAnim* anim = 0;
		for (size_t channel = 0; channel < animation->mNumMeshChannels; ++channel)
		{
			anim = animation->mMeshChannels[channel];
		}
	}
	
	//fprintf(stdout, "Loaded %zu meshes ready for drawing\n", meshes.size());
	
	
	
	Json::StyledWriter writer;
	xfile_t out = xfile_open(output_path, XF_WRITE);
	if (xfile_isopen(out))
	{
		std::string buf = writer.write(root);
		xfile_write(out, buf.c_str(), buf.length(), 1);
		xfile_close(out);
	}
}

void test_load_scene(const char* asset_root, const char* input_file, const char* output_path)
{
	Assimp::Importer importer;

	StackString<2048> input_filename = asset_root;
	input_filename.append("/").append(input_file);
	
	// Bone matrices may be incorrect with aiProcess_PreTransformVertices set.
	unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;

	
	const aiScene* scene = importer.ReadFile(input_filename(), flags);
	if (scene)
	{
		if ( scene->HasMeshes() )
		{
			StackString<2048> fullpath = output_path;
			fullpath.append("/");
			fullpath = fullpath.append(input_file).remove_extension().append(".model");
			LOGV("fullpath = %s\n", fullpath());
			convert_and_write_model(scene, fullpath());
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
	
	if (arg_parse(argc, argv) != 0)
	{
		return -1;
	}
	
	//	test_function();
	test_load_scene(asset_root->string, input_file->string, output_root->string);

	core::shutdown();
	memory::shutdown();
	return 0;
}
// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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
#pragma once

#include "platform.hpp"
#include "stackstring.hpp"

#include "mathlib.h" // for glm
#include "color.hpp"
#include "renderer.hpp"

namespace assets
{
	// Asset utils
	enum AssetType
	{
		AssetUnknown = 0,
		TextureAsset,
		MaterialAsset,
		MeshAsset,
		ShaderAsset,
		SoundAsset,
	}; // AssetType
	
	
	// called to initialize default textures and other required resources.
	void startup();
	
	// purge all assets
	void purge();
	
	// purge all assets and reclaim unused memory
	void shutdown();
	
	// Given a relative path to an asset, convert it to an absolute path and tack on file extension
	// "sounds/handy" -> "<content_directory>/sounds/handy.<platform_extension>"
	void append_asset_extension( AssetType type, StackString< MAX_PATH_SIZE > & path );
	
	typedef unsigned int AssetID;
	
	struct Asset
	{
		assets::AssetID asset_id;
		virtual ~Asset() {}
		virtual void release() = 0;
	}; // Asset
	
}; // namespace assets

namespace assets
{
	// -------------------------------------------------------------
	// Texture
	struct Texture : public virtual Asset
	{
		char * path;
		unsigned int texture_id;
		unsigned int width;
		unsigned int height;
		unsigned int flags;
		
		virtual void release();
	};
	
	
	// load a texture from disk or cache. if reload_from_disk is false, cache is preferred
	Texture * load_texture( const char * path, unsigned int flags = 0, bool ignore_cache = false );
	//	Texture * load_cubemap( const char * basename, unsigned int flags = 0, bool ignore_cache = false );
	
	// -------------------------------------------------------------
	// Shader
	struct KeyHashPair
	{
		StackString<32> key;
		int value;
	};
	
	struct Shader : public virtual Asset, public virtual renderer::ShaderParameters, public virtual renderer::ShaderProgram
	{		
		Shader();
		~Shader();
		
		int get_uniform_location( const char * name );
		virtual void release();
		
		void create_program();
		void destroy_program();
		void attach_shader( /*GLObject shader*/ );
		void bind_attributes();
		bool link_and_validate();
		void bind_uniforms();
	}; // Shader
	
	struct ShaderPermutationGroup
	{
		StackString<64> name;
		
		unsigned int num_defines;
		unsigned int num_attributes;
		unsigned int num_uniforms;
		
		StackString<64> * defines;
		StackString<64> * attributes;
		StackString<64> * uniforms;
		
		unsigned int mask_value;
		
		~ShaderPermutationGroup();
	}; // ShaderPermutationGroup
	
	
	struct ShaderPermutations
	{
		ShaderPermutationGroup base;
		
		unsigned int num_permutations;
		ShaderPermutationGroup ** options;
		
		unsigned int num_attributes;
		ShaderPermutationGroup * attributes;
		
		unsigned int num_uniforms;
		ShaderPermutationGroup * uniforms;

		ShaderPermutations();
		~ShaderPermutations();
	}; // ShaderPermutations
	
	ShaderPermutations & shader_permutations();
	void compile_shader_permutations();
	Shader * find_compatible_shader( unsigned int attributes );
	
	// -------------------------------------------------------------
	// Material
	
	// must also make a change in: MaterialParameterTypeToRenderState
	enum MaterialParameterType
	{
		MP_INT = 0,
		MP_SAMPLER_2D,
		MP_SAMPLER_CUBE,
		MP_VEC4
	};
	
	struct Material : public virtual Asset
	{
		struct Parameter
		{
			StackString<64> name;
			StackString<64> value;
			unsigned int type; // MaterialParameterType
			int intValue;
			glm::vec4 vecValue;
			unsigned int texture_unit;
		}; // Parameter
		
		enum
		{
			BLENDING = 1,
			SHADOWMAP = 2,
			CUBEMAP = 4,
		};
		
		StackString<128> name;
		Shader * shader;
		Parameter * parameters;
		unsigned int texture_id;
		unsigned int flags;
		unsigned int num_parameters;
		unsigned int requirements; // used to lookup the correct shader permutation for this material
		virtual void release();
		unsigned int Id() const;
		
		// this will generate a value based on the parameters applied
		// to this material such that the correct shader can be found and used when rendering
		void calculate_requirements();
	}; // Material
	
	Material * material_by_id( unsigned int id );
	unsigned int materialIdByName( const char * name );
	Material * load_material( const char * path, unsigned int flags = 0, bool ignore_cache = false );
	Material * default_material();
	void insertMaterial( const char * name, assets::Material * material );
	unsigned int findParameterMask( StackString<64> & name );
	unsigned int textureUnitForMap( StackString<64> & name );
	
	unsigned int materialTypeToParameterType( const char * name );
	int material_parameter_type_to_render_state( unsigned int type );
	
	// -------------------------------------------------------------
	// Mesh
	
	struct Geometry : public renderer::Geometry
	{		
		StackString<128> name;
		unsigned int material_id;
				
		Geometry();
		~Geometry();

		// set this geometry up for rendering
		void render_setup();
		
	}; // Geometry
	
	
	struct Mesh : public virtual Asset
	{
		unsigned short total_geometry;
		Geometry * geometry;
		Geometry * geometry_vn;
		glm::mat4 world_matrix;
		StackString<MAX_PATH_SIZE> path;
		
		Mesh();
		void init();
		void alloc( unsigned int num_geometry );
		void purge();
		virtual void release();
		
		// prepare all geometry
		void prepare_geometry();
		
		// upload all geometry
//		void upload_geometry();
	}; // Mesh
	
	
	typedef void (*MeshIterator)( Mesh * mesh, void * userdata );
	unsigned int get_total_meshes();
	
	void for_each_mesh( MeshIterator fn, void * userdata = 0 );
	
	Mesh * load_mesh( const char * filename, unsigned int flags = 0, bool ignore_cache = false );
	Mesh * mesh_by_name( const char * filename );
	
	// inserts a mesh from an object and associate it with a filename
	void insert_mesh( const char * filename, Mesh * ptr );
}; // namespace assets
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
#include "typedefs.h"
#include "assets.hpp"
#include "assetlibrary.hpp"
#include "kernel.hpp"
#include "image.hpp"
#include "log.h"

#include "xstr.h"
#include "xtime.h"
#include "memory.hpp"

#include "filesystem.hpp"
#include "configloader.hpp" // used to load various file formats
#include "renderer.hpp"

using namespace renderer;

namespace assets
{
	// -------------------------------------------------------------
	// Textures
	typedef AssetLibrary< Texture, TextureAsset> TextureAssetLibrary;
	TextureAssetLibrary * texture_lib;
	Texture * _default_texture = 0;
	
	void Texture::release()
	{
		image::driver_release_image( this->texture_id );
	} // release
	
	AssetLoadStatus texture_load_callback( const char * path, Texture * texture, unsigned int flags )
	{
		unsigned int texture_id;
		unsigned int width;
		unsigned int height;
		
		bool load_result = 0;
		
		if ( !(flags & image::F_CUBEMAP) ) // load 2d texture
		{
			load_result = image::load_image_from_file( path, texture_id, flags, &width, &height );
		}
		else // load cubemap
		{
			StackString< MAX_PATH_SIZE > fullpath[6];
			const char ext[][4] = { "_rt", "_lt", "_up", "_dn", "_ft", "_bk" };
			const char * names[6];
			for( int i = 0; i < 6; ++i )
			{
				fullpath[i] = path;
				fullpath[i].remove_extension();
				fullpath[i].append( ext[i] );
				assets::append_asset_extension( TextureAsset, fullpath[i] );
				names[i] = fullpath[i]();
			}
			
//			load_result = renderlib::LoadCubemap( names, texture_id, flags, &width, &height );
			assert( 0 );
		}
		
		if ( load_result )
		{
			texture->flags = flags;
			texture->texture_id = texture_id;
			texture->width = width;
			texture->height = width;
			return assets::AssetLoad_Success;
		}
		
		return assets::AssetLoad_Failure;
	} // texture_load_callback
	
	
	Texture * load_texture( const char * path, unsigned int flags, bool ignore_cache )
	{
		assert( texture_lib != 0 );
		Texture * texture = texture_lib->load_from_path( path, flags, ignore_cache );
		if ( texture )
		{
			return texture;
		}
		
		return _default_texture;
	} // load_texture
	
	// -------------------------------------------------------------
	// Shader
	Shader::Shader()
	{
		
	}
	
	Shader::~Shader()
	{
		
	}
	
	void Shader::release() {}
	
	
	ShaderPermutationGroup::~ShaderPermutationGroup()
	{
		if ( num_defines )
		{
			DESTROY_ARRAY( StackString<64>, defines, num_defines );
			num_defines = 0;
		}
		
		if ( num_attributes )
		{
			DESTROY_ARRAY( StackString<64>, attributes, num_attributes );
			num_attributes = 0;
		}
		
		if ( num_uniforms )
		{
			DESTROY_ARRAY( StackString<64>, uniforms, num_uniforms );
			num_uniforms = 0;
		}
	}
	
	
	ShaderPermutations::ShaderPermutations()
	{
		options = 0;
		attributes = 0;
		num_attributes = 0;
		uniforms = 0;
		num_uniforms = 0;
		num_permutations = 0;
	}
	
	ShaderPermutations::~ShaderPermutations()
	{
		if ( num_permutations )
		{
			if ( options )
			{
				DEALLOC(options);
			}
			
			DESTROY_ARRAY(ShaderPermutationGroup, attributes, num_attributes );
			DESTROY_ARRAY(ShaderPermutationGroup, uniforms, num_uniforms );
		}
	}
	
	
	
	
	
	
	
	assets::Shader * _shader_programs = 0;
	ShaderPermutations * _shader_permutations = 0;
	unsigned int total_shaders = 0;
	
	ShaderPermutations & shader_permutations()
	{
		assert( _shader_permutations != 0 );
		return *_shader_permutations;
	} // shader_permutations

	void readAttributesAndUniforms( ShaderPermutationGroup * option, Json::Value & root )
	{
#if 1
		Json::Value define_list = root.get( "defines", Json::nullValue );
		option->defines = 0;
		option->num_defines = define_list.size();
		if ( !define_list.isNull() )
		{
			option->defines = CREATE_ARRAY( StackString<64>, option->num_defines );
			
			Json::ValueIterator it = define_list.begin();
			Json::ValueIterator end = define_list.end();
			
			unsigned int id = 0;
			for( ; it != end; ++it, ++id )
			{
				Json::Value value = (*it);
				option->defines[ id ] = value.asString().c_str();
				LOGV( "define: %s\n", option->defines[id]() );
			}
		}
		else
		{
			LOGV( "No defines list.\n" );
		}
#endif
		
		Json::Value mask_value = root.get( "mask_value", Json::nullValue );
		if ( !mask_value.isNull() )
		{
			option->mask_value = mask_value.asInt();
			LOGV( "mask_value: %i\n", option->mask_value );
		}
		else
		{
			LOGW( "missing mask value for permutation %s\n", option->name() );
		}
		
		Json::Value attribute_list = root.get( "attributes", Json::nullValue );
		option->attributes = 0;
		option->num_attributes = attribute_list.size();
		if ( !attribute_list.isNull() )
		{
			option->attributes = CREATE_ARRAY( StackString<64>, option->num_attributes );
			
			Json::ValueIterator it = attribute_list.begin();
			Json::ValueIterator end = attribute_list.end();
			
			unsigned int attrib_id = 0;
			for( ; it != end; ++it, ++attrib_id )
			{
				Json::Value value = (*it);
				option->attributes[ attrib_id ] = value.asString().c_str();
			}
		}
		else
		{
			LOGV( "No attributes list.\n" );
		}
		
		
		Json::Value uniform_list = root.get( "uniforms", Json::nullValue );
		option->uniforms = 0;
		option->num_uniforms = uniform_list.size();
		if ( !uniform_list.isNull() )
		{
			option->uniforms = CREATE_ARRAY( StackString<64>, option->num_uniforms );
			
			Json::ValueIterator it = uniform_list.begin();
			Json::ValueIterator end = uniform_list.end();
			
			unsigned int uniform_id = 0;
			for( ; it != end; ++it, ++uniform_id )
			{
				Json::Value value = (*it);
				option->uniforms[ uniform_id ] = value.asString().c_str();
			}
		}
		else
		{
			LOGV( "No uniforms list.\n" );
		}
	} // readAttributesAndUniforms


	void readAttributesAndUniforms2( ShaderPermutationGroup * option, Json::Value & root )
	{
#if 1
		Json::Value define_list = root.get( "defines", Json::nullValue );
		option->defines = 0;
		option->num_defines = define_list.size();
		if ( !define_list.isNull() )
		{
			option->defines = CREATE_ARRAY( StackString<64>, option->num_defines );
			
			Json::ValueIterator it = define_list.begin();
			Json::ValueIterator end = define_list.end();
			
			unsigned int id = 0;
			for( ; it != end; ++it, ++id )
			{
				Json::Value value = (*it);
				option->defines[ id ] = value.asString().c_str();
				LOGV( "define: %s\n", option->defines[id]() );
			}
		}
		else
		{
			LOGV( "No defines list.\n" );
		}
#endif
		
		
		Json::Value mask_value = root.get( "mask_value", Json::nullValue );
		if ( !mask_value.isNull() )
		{
			option->mask_value = mask_value.asInt();
			LOGV( "mask_value: %i\n", option->mask_value );
		}
		else
		{
			LOGW( "missing mask value for permutation %s\n", option->name() );
		}
		
		Json::Value attribute_list = root.get( "attributes", Json::nullValue );
		option->attributes = 0;
		option->num_attributes = attribute_list.size();
		if ( !attribute_list.isNull() )
		{
			option->attributes = CREATE_ARRAY( StackString<64>, option->num_attributes );
			
			Json::ValueIterator it = attribute_list.begin();
			Json::ValueIterator end = attribute_list.end();
			
			unsigned int attrib_id = 0;
			for( ; it != end; ++it, ++attrib_id )
			{
				Json::Value value = (*it);
				option->attributes[ attrib_id ] = value.asString().c_str();
				LOGV( "attributes: %s\n", value.asString().c_str() );
			}
		}
		else
		{
			LOGV( "No attributes list.\n" );
		}
		
		
		Json::Value uniform_list = root.get( "uniforms", Json::nullValue );
		option->uniforms = 0;
		option->num_uniforms = uniform_list.size();
		if ( !uniform_list.isNull() )
		{
			option->uniforms = CREATE_ARRAY( StackString<64>, option->num_uniforms );
			
			Json::ValueIterator it = uniform_list.begin();
			Json::ValueIterator end = uniform_list.end();
			
			unsigned int uniform_id = 0;
			for( ; it != end; ++it, ++uniform_id )
			{
				Json::Value value = (*it);
				option->uniforms[ uniform_id ] = value.asString().c_str();
				LOGV( "uniforms: %s\n", value.asString().c_str() );
			}
		}
		else
		{
			LOGV( "No uniforms list.\n" );
		}
	} // readAttributesAndUniforms2
	
	util::ConfigLoadStatus load_shader_permutations( const Json::Value & root, void * data )
	{
		ShaderPermutations * permutations = (ShaderPermutations*)data;
		
		Json::Value base = root.get( "base", Json::nullValue );
		
		if ( !base.isNull() )
		{
//			Json::ValueIterator item_iter = base.begin();
//			Json::ValueIterator item_end = base.end();
			readAttributesAndUniforms( &permutations->base, base );
		}
		
#if 0
		// look at permutations
		Json::Value permutation_root = root.get("permutations", Json::nullValue );
		LOGV( "num: %i\n", permutation_root.size() );
		permutations->num_options = permutation_root.size();
		permutations->options = new ShaderPermutationGroup[ permutations->num_options ];
		Json::ValueIterator item_iter = permutation_root.begin();
		Json::ValueIterator item_end = permutation_root.end();
		unsigned int option_id = 0;
		for( ; item_iter != item_end; ++item_iter, ++option_id )
		{
			ShaderPermutationGroup * option = &permutations->options[ option_id ];
			LOGV( "loading %s...\n", item_iter.key().asString().c_str() );
			option->name = item_iter.key().asString().c_str();
			readAttributesAndUniforms( option, (*item_iter) );
		}
#endif
		
		Json::Value attributes = root.get( "attributes", Json::nullValue );
		if ( !attributes.isNull() )
		{
			Json::ValueIterator item_iter = attributes.begin();
			Json::ValueIterator item_end = attributes.end();
			permutations->num_attributes = attributes.size();
			permutations->attributes = CREATE_ARRAY( ShaderPermutationGroup, permutations->num_attributes );
			unsigned int option_id = 0;
			for( ; item_iter != item_end; ++item_iter, ++option_id )
			{
				ShaderPermutationGroup * option = &permutations->attributes[ option_id ];
				LOGV( "loading %s...\n", item_iter.key().asString().c_str() );
				option->name = item_iter.key().asString().c_str();
				readAttributesAndUniforms2( option, (*item_iter) );
			}
		}
		
		Json::Value uniforms = root.get( "uniforms", Json::nullValue );
		if ( !uniforms.isNull() )
		{
			Json::ValueIterator item_iter = uniforms.begin();
			Json::ValueIterator item_end = uniforms.end();
			permutations->num_uniforms = uniforms.size();
			permutations->uniforms = CREATE_ARRAY( ShaderPermutationGroup, permutations->num_uniforms );
			unsigned int option_id = 0;
			for( ; item_iter != item_end; ++item_iter, ++option_id )
			{
				ShaderPermutationGroup * option = &permutations->uniforms[ option_id ];
				LOGV( "loading %s...\n", item_iter.key().asString().c_str() );
				option->name = item_iter.key().asString().c_str();
				readAttributesAndUniforms2( option, (*item_iter) );
			}
		}
		
		return util::ConfigLoad_Success;
	} // load_shader_permutations



	void compile_shader_permutations()
	{
		xtime_t t;
		xtime_startup( &t );
		float start = xtime_msec( &t );
		
		ShaderPermutations & permutations = shader_permutations();
		if ( !util::json_load_with_callback("conf/shader_permutations.conf", load_shader_permutations, &shader_permutations(), true ) )
		{
			LOGE( "Unable to load shader permutations config\n" );
		}
		
		permutations.num_permutations = permutations.num_attributes+permutations.num_uniforms;
		LOGV( "loaded %i options\n", permutations.num_permutations );
		
		unsigned int total_permutations = (1 << permutations.num_permutations);
		LOGV( "total permutations: %i\n", total_permutations );
		
		renderer::IRenderDriver * driver = renderer::driver();
		size_t total_shader_bytes = total_permutations * sizeof(renderer::ShaderObject*);
		renderer::ShaderObject * vertex_shader = (renderer::ShaderObject*)ALLOC(total_shader_bytes);
		renderer::ShaderObject * fragment_shader = (renderer::ShaderObject*)ALLOC(total_shader_bytes);
		for( size_t shader = 0; shader < total_permutations; ++shader )
		{
			vertex_shader[ shader ] = driver->shaderobject_create( renderer::SHADER_VERTEX );
			fragment_shader[ shader ] = driver->shaderobject_create( renderer::SHADER_FRAGMENT );
		} // total_permutations
		

		permutations.options = (ShaderPermutationGroup**)ALLOC( permutations.num_permutations * sizeof(ShaderPermutationGroup*) );
		int pid = 0;
		for( int i = 0; i < permutations.num_attributes; ++i, ++pid )
		{
			permutations.options[pid] = &permutations.attributes[i];
			LOGV( "option: \"%s\", mask_value: %i\n", permutations.options[pid]->name(), permutations.options[pid]->mask_value );
		}
		
		for( int i = 0; i < permutations.num_uniforms; ++i, ++pid )
		{
			permutations.options[pid] = &permutations.uniforms[i];
			LOGV( "option: \"%s\", mask_value: %i\n", permutations.options[pid]->name(), permutations.options[pid]->mask_value );
		}


		StackString<32> shader_version;
		
		int vs_len;
		char * vs_source = fs::file_to_buffer( "shaders/uber.vert", 0, &vs_len );
		util::strip_shader_version( vs_source, shader_version );
		
		int fs_len;
		char * fs_source = fs::file_to_buffer( "shaders/uber.frag", 0, &fs_len );
		util::strip_shader_version( fs_source, shader_version );
		

		
		assets::Shader * shader;
		total_shaders = total_permutations;
		_shader_programs = CREATE_ARRAY( assets::Shader, total_shaders );
		
		// create all programs we'll need
		for( unsigned int shader_id = 0; shader_id < total_shaders; ++shader_id )
		{
			ShaderParameters params;
			renderer::ShaderProgram shaderobject = driver->shaderprogram_create( params );
			_shader_programs[ shader_id ].object = shaderobject.object;
		}
		
		for( int i = 0; i < total_permutations; ++i )
		{
			StackString<1024> preprocessor_defines;
			std::vector< StackString<64> * > attribute_list;
			std::vector< StackString<64> * > uniform_list;
			
			shader = &_shader_programs[i];
			shader->capabilities = 0;
			shader->id = i;

			// add the base attributes and uniforms
			for( int a = 0; a < permutations.base.num_attributes; ++a )
			{
				attribute_list.push_back( &permutations.base.attributes[a] );
			}
			
			for( int a = 0; a < permutations.base.num_uniforms; ++a )
			{
				uniform_list.push_back( &permutations.base.uniforms[a] );
			}
			
			LOGV( "----> permutation: %i\n", i );
			// build preprocessor defines, attributes, and uniforms
			for( int p = 0; p < permutations.num_permutations; ++p )
			{
				if ( i & (1 << p) )
				{
					ShaderPermutationGroup * option = permutations.options[p];
					for( int id = 0; id < option->num_defines; ++id )
					{
						LOGV( "option: %s\n", option->defines[id]() );
						preprocessor_defines.append( "#define " );
						preprocessor_defines.append( option->defines[id]() );
						preprocessor_defines.append( " 1\n" );
					}
					
					for( int a = 0; a < option->num_attributes; ++a )
					{
						attribute_list.push_back( &option->attributes[a] );
					}
					
					for( int a = 0; a < option->num_uniforms; ++a )
					{
						uniform_list.push_back( &option->uniforms[a] );
					}
					
					LOGV( "option: %s\n", option->name() );
					shader->capabilities |= (1 << option->mask_value);
				}
			}
			
			LOGV( "%i -> %s\n", shader->id, preprocessor_defines() );

			// load the shaders and pass the defines

			driver->shaderobject_compile(vertex_shader[i], vs_source, preprocessor_defines(), shader_version() );
			driver->shaderobject_compile(fragment_shader[i], fs_source, preprocessor_defines(), shader_version() );

			// tally these up
			unsigned int total_attributes = attribute_list.size();
			unsigned int total_uniforms = uniform_list.size();
			
			// copy attributes
			shader->attributes = CREATE_ARRAY( renderer::ShaderKeyValuePair, total_attributes );
			shader->total_attributes = total_attributes;
			for( int attrib_id = 0; attrib_id < total_attributes; ++attrib_id )
			{
				ShaderKeyValuePair * kp = &shader->attributes[ attrib_id ];
				kp->set_key( (*attribute_list[ attrib_id ])() );
				kp->second = attrib_id;
			}
			
			// setup uniforms
			shader->uniforms = CREATE_ARRAY( renderer::ShaderKeyValuePair, total_uniforms );
			shader->total_uniforms = total_uniforms;
			for( int uniform_id = 0; uniform_id < total_uniforms; ++uniform_id )
			{
				ShaderKeyValuePair * kp = &shader->uniforms[ uniform_id ];
				kp->set_key( (*uniform_list[ uniform_id ])() );
				kp->second = -1;
			}

			shader->set_frag_data_location( "out_Color" );

			// attach compiled code to program
			driver->shaderprogram_attach( *shader, vertex_shader[i] );
			driver->shaderprogram_attach( *shader, fragment_shader[i] );

			// attributes must be bound before linking the program
			driver->shaderprogram_bind_attributes( *shader, *shader );

			// Link and validate the program; spit out any log info
			driver->shaderprogram_link_and_validate( *shader );

			// loop through all uniforms and cache their locations
			driver->shaderprogram_bind_uniforms( *shader, *shader );

			// clean up
			driver->shaderobject_destroy( vertex_shader[ i ] );
			driver->shaderobject_destroy( fragment_shader[ i ] );

			attribute_list.clear();
			uniform_list.clear();
			
			driver->shaderprogram_deactivate( *shader );
		}

		// cleanup resources
		DEALLOC(vs_source);
		DEALLOC(fs_source);
		
		DEALLOC(vertex_shader);
		DEALLOC(fragment_shader);
		
		LOGV( "loaded %i permutations in %gms\n", total_shaders, xtime_msec( &t ) - start );
	} // compile_shader_permutations


	Shader * find_compatible_shader( unsigned int attributes )
	{
		Shader * shader = 0;
		for( int i = total_shaders-1; i >= 0; --i )
		{
			shader = &_shader_programs[ i ];
//			LOGV( "[%i] attributes: %i sha: %i\n", shader->id, attributes, shader->capabilities );
			if ( (attributes & shader->capabilities) == shader->capabilities )
			{
				return shader;
			}
		}
		
		LOGE( "Unable to find compatible shader!\n" );
		return 0;
	}

	// -------------------------------------------------------------
	// Material
	
	typedef AssetLibrary< Material, MaterialAsset> MaterialAssetLibrary;
	MaterialAssetLibrary * mat_lib;
	Material * _default_material = 0;
	Material * default_material()
	{
		return _default_material;
	}
	
	void Material::release()
	{
		if ( num_parameters )
		{
			DESTROY_ARRAY(Parameter, parameters, num_parameters);
		}
	}
	
	Material * material_by_id( unsigned int id )
	{
		return mat_lib->find_with_id( id );
	}
	
	unsigned int materialIdByName( const char * name )
	{
		return 0;
//		return mat_lib->getIdForName( name );
	}
	
	void insertMaterial( const char * name, assets::Material * material )
	{
		mat_lib->take_ownership( name, material );
	}
	
	enum ParamFlags
	{
		PF_TYPE = 1,
		PF_VALUE
	};
	
	bool checkParamForErrors( int src, int expected )
	{
		return true;
	}
	
	util::ConfigLoadStatus material_load_from_json( const Json::Value & root, void * data )
	{
		Material * material = (Material*)data;
		Json::Value name = root["name"];
		Json::Value texture = root["texture"];
		Json::Value type = root["type"];
		Json::Value shader = root["shader"];
		
		unsigned int texture_flags = image::F_RGBA;
		material->flags = 0;
		material->num_parameters = 0;
		material->parameters = 0;
//		material->requirements = 0;
//		int required_params = PF_TYPE | PF_VALUE;
		
		if ( !name.empty() )
		{
			material->name = name.asString().c_str();
		}
		
		if ( !type.empty() )
		{
			if ( type.asString() == "alpha" )
			{
				texture_flags |= image::F_ALPHA;
				material->flags |= Material::BLENDING;
			}
			else if ( type.asString() == "cubemap" )
			{
				texture_flags = image::F_RGBA | image::F_CLAMP;
				material->flags |= Material::CUBEMAP;
			}
		}
				
		if ( !texture.empty() )
		{
			assets::Texture * texpointer;
			
			if ( material->flags & Material::CUBEMAP )
			{
//				texpointer = assets::loadCubemap( texture.asString().c_str(), texture_flags );
			}
			else
			{
				texpointer = load_texture( texture.asString().c_str(), texture_flags );
			}
			
			if ( texpointer )
			{
				material->texture_id = texpointer->texture_id;
			}
		}
		
		
		// parse and load shader params
		Json::Value param_list = root["params"];
		LOGV( "Total Parameters: %i\n", param_list.size() );
		Json::ValueIterator piter = param_list.begin();
		Json::ValueIterator piter_end = param_list.end();
		
		material->num_parameters = param_list.size();
		if ( material->num_parameters )
		{
			material->parameters = CREATE_ARRAY( Material::Parameter, material->num_parameters );
			Material::Parameter * parameter;
			unsigned int param_id = 0;
			for( ; piter != piter_end; ++piter, ++param_id )
			{
				parameter = &material->parameters[ param_id ];
				int param_flags = 0;
				Json::Value plist = (*piter);
				parameter->name = piter.key().asString().c_str();
				LOGV( "parameter-> %s\n", parameter->name() );
				
				Json::Value type = plist.get( "type", "" );
				if ( !type.isNull() )
				{
					param_flags |= PF_TYPE;
					std::string typestr = type.asString();
					LOGV( "type: %s\n", typestr.c_str() );
					
					// convert string to param type
					parameter->type = materialTypeToParameterType( typestr.c_str() );
				}
				else
				{
					LOGE( "Couldn't find parameter field: \"type\"\n" );
					return util::ConfigLoad_Failure;
				}
				
				
				
				if ( parameter->type == MP_INT )
				{
					Json::Value value = plist.get( "value", "" );
					if ( !value.isNull() )
					{
						param_flags |= PF_VALUE;
						parameter->intValue = atoi( value.asString().c_str() );
						LOGV( "param value: %i\n", parameter->intValue );
					}
				}
				else if ( parameter->type == MP_SAMPLER_2D )
				{
					param_flags |= PF_VALUE;
					Json::Value texture_param = plist.get( "texture", Json::nullValue );
					
					if ( texture_param.isNull() )
					{
						LOGW( "texture param missing for \"sampler\" type\n" );
						param_flags &= ~PF_VALUE;
					}
					
					
					if ( param_flags & PF_VALUE )
					{
						assets::Texture * tex = assets::load_texture( texture_param.asString().c_str() );
						parameter->intValue = tex->texture_id;
						LOGV( "param value: %i\n", parameter->intValue );
						
						parameter->texture_unit = textureUnitForMap( parameter->name );
						LOGV( "texture unit: %i\n", parameter->texture_unit );
					}
					else
					{
						return util::ConfigLoad_Failure;
					}
				}
				else if ( parameter->type == MP_SAMPLER_2D )
				{
					param_flags |= PF_VALUE;
					Json::Value texture_unit = plist.get( "texture_unit", Json::nullValue );
					Json::Value texture_param = plist.get( "texture", Json::nullValue );
					
					
					if ( texture_param.isNull() )
					{
						LOGW( "texture param missing for \"sampler\" type\n" );
						param_flags &= ~PF_VALUE;
					}
					
					if ( texture_unit.isNull() )
					{
						LOGW( "texture_unit missing for \"sampler\" type\n" );
					}
					
					if ( param_flags & PF_VALUE )
					{
						assets::Texture * tex = assets::load_texture( texture_param.asString().c_str() );
						parameter->intValue = tex->texture_id;
						LOGV( "param value: %i\n", parameter->intValue );
						
						parameter->texture_unit = textureUnitForMap( parameter->name );
						LOGV( "texture unit: %i\n", parameter->texture_unit );
					}
					else
					{
						return util::ConfigLoad_Failure;
					}
				}
				else if ( parameter->type == MP_SAMPLER_CUBE )
				{
					param_flags |= PF_VALUE;
					Json::Value texture_unit = plist.get( "texture_unit", Json::nullValue );
					Json::Value texture_param = plist.get( "texture", Json::nullValue );
					
					
					if ( texture_param.isNull() )
					{
						LOGW( "texture param missing for \"samplerCube\" type\n" );
						param_flags &= ~PF_VALUE;
					}
					
					if ( texture_unit.isNull() )
					{
						LOGW( "texture_unit missing for \"samplerCube\" type\n" );
					}
					
					if ( param_flags & PF_VALUE )
					{
						texture_flags = image::F_RGBA | image::F_CLAMP;
						assets::Texture * tex = 0; //assets::loadCubemap( texture_param.asString().c_str(), texture_flags );
						parameter->intValue = tex->texture_id;
						LOGV( "param value: %i\n", parameter->intValue );
						
						parameter->texture_unit = textureUnitForMap( parameter->name );
						LOGV( "texture unit: %i\n", parameter->texture_unit );
					}
					else
					{
						return util::ConfigLoad_Failure;
					}
				}
				else if ( parameter->type == MP_VEC4 )
				{
					param_flags |= PF_VALUE;
					Json::Value value = plist.get( "value", Json::nullValue );
					if ( value.isNull() )
					{
						param_flags &= ~PF_VALUE;
						LOGW( "Unable to find value for \"vec4\" type\n" );
					}
					else
					{
						int results = sscanf( value.asString().c_str(), "%g,%g,%g,%g", &parameter->vecValue[0], &parameter->vecValue[1], &parameter->vecValue[2], &parameter->vecValue[3] );
						if ( results < 4 )
						{
							LOGW( "Unable to parse \"vec4\" type\n" );
							param_flags &= ~PF_VALUE;
						}
						else
						{
							LOGV( "parsed vec4: %g, %g, %g, %g\n", parameter->vecValue[0], parameter->vecValue[1], parameter->vecValue[2], parameter->vecValue[3] );
						}
					}
				}
				else
				{
					LOGE( "Couldn't find parameter field: \"value\"\n" );
					return util::ConfigLoad_Failure;
				}
			} // read all shader parameters
		} //  num_parameters
		
		
		calculateRequirements( material );

#if 0
		StackString< MAX_PATH_SIZE > path = "conf/";
		if ( !shader.empty() )
		{
			path.append( shader.asString().c_str() );
		}
		else
		{
//			path.append( ASSETS_DEFAULT_MATERIAL_SHADER_NAME );
		}
		
//		material->shader = loadShader( path() );
		if ( !material->shader )
		{
			LOGW( "Couldn't find shader \"%s\" for material!\n", shader.asString().c_str() );
		}
#endif
		return util::ConfigLoad_Success;
	}
	
	Material * load_material( const char * filename, unsigned int flags, bool ignore_cache )
	{
		Material * material = mat_lib->load_from_path( filename, flags, ignore_cache );
		if ( material )
		{
			return material;
		}
		return default_material();
	} // load_material
	
	AssetLoadStatus material_load_callback( const char * path, Material * material, unsigned int flags )
	{
		if (util::json_load_with_callback( path, material_load_from_json, material, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}

		return AssetLoad_Failure;
	} // material_load_callback
	
	unsigned int materialTypeToParameterType( const char * name )
	{
		if ( xstr_nicmp( name, "int", 0 ) == 0 )
		{
			return MP_INT;
		}
		else if ( xstr_nicmp( name, "samplerCube", 0 ) == 0 )
		{
			return MP_SAMPLER_CUBE;
		}
		else if ( xstr_nicmp( name, "sampler", 0 ) == 0 )
		{
			return MP_SAMPLER_2D;
		}
		else if ( xstr_nicmp( name, "vec4", 0 ) == 0 )
		{
			return MP_VEC4;
		}
		
		LOGW( "Couldn't find material parameter with name: %s\n", name );
		return 0;
	} // materialTypeToParameterType
	
	
	unsigned int findParameterMask( StackString<64> & name )
	{
		for( unsigned int option_id = 0; option_id < shader_permutations().num_permutations; ++option_id )
		{
			ShaderPermutationGroup * option = shader_permutations().options[ option_id ];
			LOGV( "comparing \"%s\" to \"%s\"\n", option->name(), name() );
			if ( xstr_nicmp( name(), option->name(), 64 ) == 0 )
			{
				LOGV( "found option \"%s\" value: %i\n", option->name(), option->mask_value );
				return 1 << option->mask_value;
			}
		}
		return 0;
	} // findParameterMask
	
	unsigned int textureUnitForMap( StackString<64> & name )
	{
		if ( xstr_nicmp( name(), "diffusemap", 0 ) == 0 )
		{
			return 0;
		}
		else if ( xstr_nicmp( name(), "normalmap", 0 ) == 0 )
		{
			return 1;
		}
		else if ( xstr_nicmp( name(), "specularmap", 0 ) == 0 )
		{
			return 2;
		}
		else if ( xstr_nicmp( name(), "cubemap", 0 ) == 0 )
		{
			return 0;
		}
		
		return 0;
	} // textureUnitForMap
	
	void calculateRequirements( Material * material )
	{
		// calculate material requirements
		material->requirements = 0;
		
		for( int id = 0; id < material->num_parameters; ++id )
		{
			Material::Parameter * param = &material->parameters[ id ];
			unsigned int mask = findParameterMask( param->name );
			LOGV( "param \"%s\" -> %i\n", param->name(), mask );
			material->requirements |= mask;
		}
		
		LOGV( "Material requirements: %i\n", material->requirements );
	}
	
	// -------------------------------------------------------------
	// Mesh
	typedef AssetLibrary< Mesh, MeshAsset> MeshAssetLibrary;
	MeshAssetLibrary * mesh_lib;

	util::ConfigLoadStatus mesh_load_from_json( const Json::Value & root, void * data )
	{
		Mesh * mesh = (Mesh*)data;
		//	LOGV( "Loading mesh %s\n", mesh->name() );
		
		assets::Material * default_mat = assets::load_material( "materials/default" );
		if ( !default_mat )
		{
			LOGE( "Could not load the default material!\n" );
			return util::ConfigLoad_Failure;
		}
		
		Json::Value materials = root["materials"];
		LOGV( "Total Materials: %i\n", materials.size() );
		
		Json::ValueIterator mit = materials.begin();

		unsigned int * material_ids = (unsigned int*)ALLOC( sizeof(unsigned int) * materials.size() );
		unsigned int current_material = 0;
		assets::Material * amat = 0;
		for( ; mit != materials.end(); ++mit ) // MEMORY LEAKS HERE
		{
			Json::Value material = (*mit);
			std::string material_name = material["name"].asString();
			
			StackString<MAX_PATH_SIZE> material_path;
			xstr_sprintf( &material_path[0], material_path.max_size(), "materials/%s", material_name.c_str() );
			amat = assets::load_material( material_path() );
			if ( amat )
			{
				material_ids[ current_material ] = amat->Id();
			}
			else
			{
				amat = assets::default_material();
				material_ids[ current_material ] = amat->Id();
			}
			LOGV( "assigned material '%s' to (%i)\n", material_name.c_str(), material_ids[ current_material ] );
			++current_material;
		}
		
		Json::Value geometry_list = root["geometry"];
		LOGV( "Total Geometry: %i\n", geometry_list.size() );
		mesh->total_geometry = geometry_list.size();
		mesh->geometry = CREATE_ARRAY( Geometry, mesh->total_geometry );
		mesh->geometry_vn = CREATE_ARRAY( Geometry, mesh->total_geometry );
		
		Geometry * geometry;
		int gid = 0;
		int vnid = 0;
		Json::ValueIterator git = geometry_list.begin();
		for( ; git != geometry_list.end(); ++git )
		{
			geometry = &mesh->geometry[ gid++ ];
			geometry->draw_type = renderer::DRAW_INDEXED_TRIANGLES;
			
			Json::Value geometry_node = *git;
			Json::Value positions = geometry_node["positions"];
			Json::Value indices = geometry_node["indices"];
			Json::Value normals = geometry_node["normals"];
			Json::Value uvs = geometry_node["uvs"];
			int material_id = geometry_node["material_id"].asInt();
			LOGV( "geometry: %i, material_id: %i\n", gid-1, material_id );
			LOGV( "# vertices: %i\n", positions.size()/3 );
			LOGV( "# indices: %i\n", indices.size() );
			LOGV( "# triangles: %i\n", indices.size()/3 );
			geometry->index_count = indices.size();
			geometry->indices = (IndexType*)ALLOC( sizeof(IndexType) * geometry->index_count );
			for( int i = 0; i < geometry->index_count; ++i )
			{
				geometry->indices[i] = indices[i].asInt();
			}
			
			geometry->vertex_count = positions.size() / 3;
			geometry->vertices = CREATE_ARRAY( glm::vec3, geometry->vertex_count );
			geometry->normals = CREATE_ARRAY( glm::vec3, geometry->vertex_count );
			geometry->uvs = CREATE_ARRAY( UV, geometry->vertex_count );
			
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->vertices[v] = glm::vec3(positions[v*3].asFloat(), positions[v*3+1].asFloat(), positions[v*3+2].asFloat() );
			}
			
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->normals[v] = glm::vec3(normals[v*3].asFloat(), normals[v*3+1].asFloat(), normals[v*3+2].asFloat() );
			}
			
			if ( uvs.size() > 0 )
			{
				for( int v = 0; v < geometry->vertex_count; ++v )
				{
					geometry->uvs[v].u = uvs[v*2].asFloat();
					geometry->uvs[v].v = uvs[v*2+1].asFloat();
				}
			}
			else
			{
				LOGW( "Mesh has no UV coordinates!\n" );
			}
			
#if 0
			geometry->colors = new aengine::Color[ geometry->vertex_count ];
			for( int v = 0; v < geometry->vertex_count; ++v )
			{
				geometry->colors[v] = aengine::Color( 255, 255, 255, 255 );
			}
#endif
			
			
			if ( material_id != -1 && current_material > 0 /*&& material_id < current_material*/ )
			{
				geometry->material_id = material_ids[ material_id ];
				LOGV( "using material %i %i\n", material_id, geometry->material_id );
			}
			else
			{
				Material * default_material = assets::default_material();
				geometry->material_id = default_material->Id();
			}
			
#if 0
			for( int i = 0; i < geometry->index_count; ++i )
			{
				int idx = geometry->indices[i];
				//			LOGV( "%i, (%g %g %g), (%g %g %g), (%g %g)\n",
				//				   idx,
				//				   geometry->vertices[ idx ][0], geometry->vertices[ idx ][1], geometry->vertices[ idx ][2],
				//				   geometry->normals[ idx ][0], geometry->normals[ idx ][1], geometry->normals[ idx ][2],
				//				   geometry->uvs[ idx ].u, geometry->uvs[ idx ].v );
				Vector3 v = geometry->vertices[ idx ];
				Vector3 n = geometry->normals[ idx ];
				UV uv = geometry->uvs[ idx ];
				LOGV( "i %i: %i = %g %g %g | %g %g %g | %g %g\n", i, idx, v[0], v[1], v[2], n[0], n[1], n[2], uv.u, uv.v );
			}
#endif
			
#if 1		// THERE ARE LEAKS IN HERE
			//
			// setup debug normals now
			
			if ( geometry->normals )
			{
				
				Geometry * vertex_normals = &mesh->geometry_vn[ vnid++ ];
				vertex_normals->draw_type = renderer::DRAW_LINES;
				if ( default_mat )
				{
					vertex_normals->material_id = default_mat->Id();
				}
				
				const float normal_scale = 0.25f;
				Color vertex_normal_color(255, 0, 255);
				vertex_normals->vertex_count = geometry->vertex_count * 2;
				vertex_normals->index_count = vertex_normals->vertex_count;
				vertex_normals->vertices = CREATE_ARRAY(glm::vec3, vertex_normals->vertex_count );
				vertex_normals->indices = CREATE_ARRAY(IndexType, vertex_normals->index_count );
				vertex_normals->colors = CREATE_ARRAY(Color, vertex_normals->vertex_count );
				vertex_normals->normals = 0;
				vertex_normals->uvs = 0;
				
				unsigned int vid;
				for( int v = 0; v < geometry->vertex_count; ++v )
				{
					vid = v*2;
					glm::vec3 start = geometry->vertices[v];
					glm::vec3 end = (start + geometry->normals[v] * normal_scale);
					vertex_normals->vertices[ vid ] = start;
					vertex_normals->vertices[ vid+1 ] = end;
					vertex_normals->indices[ vid ] = vid;
					vertex_normals->indices[ vid+1 ] = vid+1;
					vertex_normals->colors[ vid ] = vertex_normal_color;
					vertex_normals->colors[ vid+1 ] = vertex_normal_color;
				}
			}
#endif			
		}
		
		DEALLOC( material_ids );
		
		return util::ConfigLoad_Success;
	} // mesh_load_from_json
	
	
	Geometry::Geometry()
	{
		vertices = 0;
		normals = 0;
		uvs = 0;
		indices = 0;
		material_id = 0;
		colors = 0;
		
		vertex_count = 0;
		index_count = 0;
		draw_type = renderer::DRAW_TRIANGLES;
//		render_data = 0;

		attributes = 0;
		is_animated = 0;
		vertexbuffer = 0;
	}
	
	Geometry::~Geometry()
	{
		using namespace glm;
		if ( vertices )
		{
			DESTROY_ARRAY( vec3, vertices, vertex_count );
			vertices = 0;
		}
		
		if ( normals )
		{
			DESTROY_ARRAY( vec3, normals, vertex_count );
			normals = 0;
		}
		
		if ( colors )
		{
			DESTROY_ARRAY( Color, colors, vertex_count );
			colors = 0;
		}
		
		if ( uvs )
		{
			DESTROY_ARRAY( UV, uvs, vertex_count );
			uvs = 0;
		}
		
		if ( indices )
		{
			DEALLOC( indices );
			indices = 0;
		}
		
		if ( this->vertexbuffer )
		{
			renderer::driver()->vertexbuffer_destroy( this->vertexbuffer );
		}
	}
	
#if 0
	void Geometry::alloc_vertices( unsigned int num_vertices )
	{
		vertex_count = num_vertices;
		vertices = CREATE_ARRAY( glm::vec3, num_vertices );
	} // alloc_vertices
	
	void Geometry::alloc_indices( unsigned int num_indices )
	{
		index_count = num_indices;
		indices = CREATE_ARRAY( renderer::IndexType, num_indices );
	} // alloc_indices
#endif

	void Geometry::render_setup()
	{
		VertexDescriptor descriptor;
		// if we already setup this geometry; skip
		if ( attributes > 0 )
		{
			return;
		}
		
		// always has at least a position
		descriptor.add( VD_FLOAT3 );
		
		if ( normals )
		{
			attributes |= (1 << GV_NORMAL);
			descriptor.add( VD_FLOAT3 );
		}
		
		if ( colors )
		{
			attributes |= (1 << GV_COLOR);
			descriptor.add( VD_UNSIGNED_BYTE4 );
		}
		
		if ( uvs )
		{
			attributes |= (1 << GV_UV0);
			descriptor.add( VD_FLOAT2 );
		}
		
		this->vertexbuffer = renderer::driver()->vertexbuffer_from_geometry( descriptor, this );
		
		if ( !this->is_animated )
		{
			renderer::driver()->vertexbuffer_upload_geometry( this->vertexbuffer, this );
		}
//		vertexstream.create( this->vertex_count, this->index_count, renderer::BUFFER_STATIC );
//		this->vertexbuffer = renderer::driver()->vertexbuffer_create( descriptor, this->draw_type, renderer::BUFFER_STATIC, descriptor.calculate_vertex_stride(), this->vertex_count, this->index_count );
	}
	
	Mesh::Mesh()
	{
		init();
	} // Mesh
	
	void Mesh::init()
	{
		total_geometry = 0;
		geometry = 0;
		geometry_vn = 0;
	} // init
	
	void Mesh::alloc( unsigned int num_geometry )
	{
		total_geometry = num_geometry;
//		geometry = CREATE_ARRAY( Geometry,  num_geometry );
	} // alloc
	
	void Mesh::purge()
	{		
		DESTROY_ARRAY( Geometry, geometry, total_geometry );
		geometry = 0;
		
		DESTROY_ARRAY( Geometry, geometry_vn, total_geometry );
		geometry_vn = 0;
		
		init();
	} // purge
	
	void Mesh::release()
	{
		purge();
	} // release
	
	void Mesh::prepare_geometry()
	{
		for( unsigned int geo_id = 0; geo_id < total_geometry; ++geo_id )
		{
			assets::Geometry * g = &geometry[ geo_id ];
			g->render_setup();
		}
	} // prepare_geometry
	
	unsigned int get_total_meshes()
	{
		return mesh_lib->total_asset_count();
	} // get_total_meshes
	
	Mesh * load_mesh( const char * filename, unsigned int flags, bool ignore_cache )
	{
		return mesh_lib->load_from_path( filename, 0, ignore_cache );
	} // load_mesh
		
	void for_each_mesh( MeshIterator fn, void * userdata )
	{
		if ( !fn )
		{
			LOGE( "Invalid mesh iterator function!\n" );
			return;
		}
		
		mesh_lib->foreach_asset( fn, userdata );
	} // for_each_mesh
	
	
	void insert_mesh( const char * filename, Mesh * ptr )
	{
		if ( !mesh_lib->find_with_path( filename ) )
		{
			mesh_lib->take_ownership( filename, ptr );
		}
	} // insert_mesh
	
	Mesh * mesh_by_name( const char * filename )
	{
		return mesh_lib->find_with_path( filename );
	} // mesh_by_name
		

	
	AssetLoadStatus mesh_load_callback( const char * path, Mesh * mesh, unsigned int flags )
	{
		if ( util::json_load_with_callback(path, mesh_load_from_json, mesh, true ) == util::ConfigLoad_Success )
		{
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // mesh_load_callback
	
}; // namespace assets


namespace assets
{
	void startup()
	{
		// allocate asset libraries
		texture_lib = CREATE(TextureAssetLibrary, texture_load_callback);
		mesh_lib = CREATE(MeshAssetLibrary, mesh_load_callback);
		mat_lib = CREATE(MaterialAssetLibrary, material_load_callback );
		
		// setup default texture
		_default_texture = texture_lib->allocate_asset();
		_default_texture->texture_id = image::load_default_texture();
		texture_lib->take_ownership("textures/default", _default_texture);
		LOGV( "Loaded default texture; id = %i\n", _default_texture->texture_id );
			
		// load shader permutations
		_shader_permutations = CREATE( ShaderPermutations );
		compile_shader_permutations();
		
		// setup default material
		_default_material = mat_lib->allocate_asset();
		_default_material->texture_id = _default_texture->texture_id;
		_default_material->name = "default";
		_default_material->num_parameters = 1;
		Material::Parameter * parameter = _default_material->parameters = CREATE(Material::Parameter);
		parameter->name = "diffusemap";
		parameter->type = MP_SAMPLER_2D;
		parameter->texture_unit = textureUnitForMap( parameter->name );
		parameter->intValue = _default_texture->texture_id;
		calculateRequirements( _default_material );
		mat_lib->take_ownership( "materials/default", _default_material );
		
	} // startup
	
	void purge()
	{
	
		for( unsigned int shader_id = 0; shader_id < total_shaders; ++shader_id )
		{
			ShaderParameters params;
			renderer::driver()->shaderprogram_destroy( _shader_programs[ shader_id ] );
		}
	
		DESTROY_ARRAY( Shader, _shader_programs, total_shaders );
		_shader_programs = 0;
		DESTROY( ShaderPermutations, _shader_permutations );
		
	
		texture_lib->release_and_purge();
		mesh_lib->release_and_purge();
		mat_lib->release_and_purge();
	} // purge
	
	void shutdown()
	{
		purge();
		DESTROY(TextureAssetLibrary, texture_lib);
		DESTROY(MeshAssetLibrary, mesh_lib);
		DESTROY(MaterialAssetLibrary, mat_lib);
	} // shutdown

	void append_asset_extension( AssetType type, StackString<MAX_PATH_SIZE> & path )
	{
		const char * extension = "";
		kernel::KernelDeviceFlags device_flags = kernel::instance()->parameters().device_flags;
		
		switch( type )
		{
			case SoundAsset:
			{
#if PLATFORM_IS_MOBILE
				if ( (device_flags & kernel::DeviceiPad) || (device_flags & kernel::DeviceiPhone) )
				{
					extension = "caf";
				}
#else
				;
				extension = "ogg";
#endif
				break;
			} // SoundAsset
			
			case TextureAsset:
			{
				if ( device_flags & kernel::DeviceDesktop )
				{
					// ...
					extension = "png";
				}
				else if ( device_flags & kernel::DeviceiPhone )
				{
					extension = "png";
					path.append( "-iphone" );
				}
				else if ( device_flags & kernel::DeviceiPad )
				{
					extension = "png";
					path.append( "-ipad" );
				}
				
				if ( device_flags & kernel::DeviceSupportsRetinaDisplay )
				{
					path.append( "@2x" );
				}
				
				break;
			} // TextureAsset
			
			
			case MaterialAsset:
			{
				extension = "material";
				break;
			} // MaterialAsset
			
			case MeshAsset:
			{
				extension = "model";
				break;
			} // MeshAsset
			
			default: LOGW( "AssetType %i is NOT supported!\n" ); break;
		}
		
		path.append( "." );
		path.append( extension );
		
	} // append_asset_extension
		
}; // namespace assets
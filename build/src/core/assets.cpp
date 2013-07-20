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
	// Shader
	Shader::Shader()
	{
		
	}
	
	Shader::~Shader()
	{
		
	}
	
	
	int Shader::get_uniform_location( const char * name )
	{
//		LOGV( "# uniforms: %i\n", total_uniforms );
		for( int i = 0; i < total_uniforms; ++i )
		{
			
			if ( xstr_nicmp( uniforms[i].first, name, xstr_len(uniforms[i].first) ) == 0 )
			{
//				LOGV( "uniform: %s, at %i\n", name, uniforms[i].second );
				return uniforms[i].second;
			}
		}
		
		LOGW( "No uniform named %s (%i)\n", name, id );
		return -1;
	} // get_uniform_location
	
	void Shader::release() {}
	
	ShaderPermutationGroup::ShaderPermutationGroup()
	{
		num_defines = 0;
		num_attributes = 0;
		num_uniforms = 0;
		num_requires = 0;
		num_conflicts = 0;
	}
	
	ShaderPermutationGroup::~ShaderPermutationGroup()
	{
		if ( num_defines )
		{
			DESTROY_ARRAY( ShaderString, defines, num_defines );
			num_defines = 0;
		}
		
		if ( num_attributes )
		{
			DESTROY_ARRAY( ShaderString, attributes, num_attributes );
			num_attributes = 0;
		}
		
		if ( num_uniforms )
		{
			DESTROY_ARRAY( ShaderString, uniforms, num_uniforms );
			num_uniforms = 0;
		}
		
		if ( num_requires )
		{
			DESTROY_ARRAY( ShaderString, requires, num_requires );
			num_requires = 0;
		}
		
		if ( num_conflicts )
		{
			DESTROY_ARRAY( ShaderString, conflicts, num_conflicts );
			num_conflicts = 0;
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
		num_parameters = 0;
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
	
	void read_string_array( ShaderString ** array, unsigned int & num_items, Json::Value & root )
	{
		num_items = root.size();
		*array = CREATE_ARRAY( ShaderString, num_items );
		Json::ValueIterator it = root.begin();
		Json::ValueIterator end = root.end();
		
		unsigned int id = 0;
		for( ; it != end; ++it, ++id )
		{
			Json::Value value = (*it);
			(*array)[ id ] = value.asString().c_str();
		}
	} // read_string_array

	void print_string_array( const char * name, ShaderString * array, unsigned int num_items )
	{
		LOGV( "\"%s\" items:\n", name );
		for( unsigned int i = 0; i < num_items; ++i )
		{
			LOGV( "\t%s\n", array[i].c_str() );
		}
	}

	void read_permutation_group( Json::Value & root, ShaderPermutationGroup * option, bool read_as_parameter )
	{
		Json::Value define_list = root.get( "defines", Json::nullValue );
		option->defines = 0;
		option->num_defines = define_list.size();
		if ( !define_list.isNull() )
		{
			read_string_array( &option->defines, option->num_defines, define_list );
			//print_string_array( "defines", option->defines, option->num_defines );
		}

		if ( read_as_parameter )
		{
			// read as another shader parameter and increment parameter index
			option->mask_value = shader_permutations().num_parameters++;
		}
		
		Json::Value attribute_list = root.get( "attributes", Json::nullValue );
		option->attributes = 0;
		option->num_attributes = attribute_list.size();
		if ( !attribute_list.isNull() )
		{
			read_string_array( &option->attributes, option->num_attributes, attribute_list );
			//print_string_array( "attributes", option->attributes, option->num_attributes );
		}

		Json::Value uniform_list = root.get( "uniforms", Json::nullValue );
		option->uniforms = 0;
		option->num_uniforms = uniform_list.size();
		if ( !uniform_list.isNull() )
		{
			read_string_array( &option->uniforms, option->num_uniforms, uniform_list );
			//print_string_array( "uniforms", option->uniforms, option->num_uniforms );
		}
		
		Json::Value require_list = root.get( "requires", Json::nullValue );
		if ( !require_list.isNull() )
		{
			read_string_array( &option->requires, option->num_requires, require_list );
			//print_string_array( "requires", option->requires, option->num_requires );
		}
		
		Json::Value conflicts_list = root.get( "conflicts", Json::nullValue );
		if ( !conflicts_list.isNull() )
		{
			read_string_array( &option->conflicts, option->num_conflicts, conflicts_list );
			//print_string_array( "conflicts", option->conflicts, option->num_conflicts );
		}
	} // read_permutation_group
	
	void read_permutation_section( Json::Value & root, ShaderPermutationGroup ** section, unsigned int & num_items )
	{
		Json::ValueIterator item_iter = root.begin();
		Json::ValueIterator item_end = root.end();
		num_items = root.size();
		*section = CREATE_ARRAY( ShaderPermutationGroup, num_items );
		unsigned int item_id = 0;
		for( ; item_iter != item_end; ++item_iter, ++item_id )
		{
			ShaderPermutationGroup * option = &(*section)[ item_id ];
			option->name = item_iter.key().asString().c_str();
			read_permutation_group( (*item_iter), option, true );
		}
	} // read_permutation_section
	
	util::ConfigLoadStatus load_shader_permutations( const Json::Value & root, void * data )
	{
		ShaderPermutations * permutations = (ShaderPermutations*)data;
		
		Json::Value base = root.get( "base", Json::nullValue );
		if ( !base.isNull() )
		{
			read_permutation_group( base, &permutations->base, false );
		}

		Json::Value attributes = root.get( "attributes", Json::nullValue );
		if ( !attributes.isNull() )
		{
			read_permutation_section( attributes, &permutations->attributes, permutations->num_attributes );
		}
		
		Json::Value uniforms = root.get( "uniforms", Json::nullValue );
		if ( !uniforms.isNull() )
		{
			read_permutation_section( uniforms, &permutations->uniforms, permutations->num_uniforms );
		}
		
		Json::Value frag_data_location = root.get( "frag_data_location", Json::nullValue );
		permutations->frag_location = frag_data_location.asString().c_str();
		
		return util::ConfigLoad_Success;
	} // load_shader_permutations

	void compile_shader_permutations()
	{
		xtime_t t;
		xtime_startup( &t );
		float start = xtime_msec( &t );
		
		_shader_permutations = CREATE(ShaderPermutations);
		
#if PLATFORM_IS_MOBILE
		const char permutations_config[] = "conf/mobile_permutations.conf";
#else
		const char permutations_config[] = "conf/shader_permutations.conf";
#endif
		
		ShaderPermutations & permutations = shader_permutations();
		if ( !util::json_load_with_callback(permutations_config, load_shader_permutations, &shader_permutations(), true))
		{
			LOGE( "Unable to load shader permutations config\n" );
		}
		
		permutations.num_permutations = permutations.num_attributes+permutations.num_uniforms;
//		LOGV( "loaded %i options\n", permutations.num_permutations );
		
		unsigned int total_permutations = (1 << permutations.num_permutations);
//		LOGV( "total permutations: %i\n", total_permutations );
		
		renderer::IRenderDriver * driver = renderer::driver();
		
		renderer::ShaderProgram shader_program;
		shader_program.object = 0;
		driver->shaderprogram_deactivate( shader_program );
				
		permutations.options = (ShaderPermutationGroup**)ALLOC( permutations.num_permutations * sizeof(ShaderPermutationGroup*) );
		int pid = 0;
		for( int i = 0; i < permutations.num_attributes; ++i, ++pid )
		{
			permutations.options[pid] = &permutations.attributes[i];
//			LOGV( "option: \"%s\", mask_value: %i\n", permutations.options[pid]->name.c_str(), permutations.options[pid]->mask_value );
		}
		
		for( int i = 0; i < permutations.num_uniforms; ++i, ++pid )
		{
			permutations.options[pid] = &permutations.uniforms[i];
//			LOGV( "option: \"%s\", mask_value: %i\n", permutations.options[pid]->name.c_str(), permutations.options[pid]->mask_value );
		}


		StackString<32> shader_version;
		
		int vs_len;
		char * vs_source = fs::file_to_buffer( "shaders/uber.vert", 0, &vs_len );
		util::strip_shader_version( vs_source, shader_version );
		
		int fs_len;
		char * fs_source = fs::file_to_buffer( "shaders/uber.frag", 0, &fs_len );
		util::strip_shader_version( fs_source, shader_version );
		if ( shader_version._length == 0 )
		{
#if defined(PLATFORM_IS_MOBILE) || defined(PLATFORM_IS_RASPBERRYPI)
			shader_version = "#version 100";
#else
			shader_version = "#version 150";
#endif
			LOGW( "Unable to extract version from shader! Forcing to %s.\n", shader_version() );
		}
		shader_version.append( "\n" );
		
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
			// StackString<1024> preprocessor_defines;
			std::string preprocessor_defines;
			std::vector< ShaderString *, GeminiAllocator<ShaderString*> > attribute_list;
			std::vector< ShaderString *, GeminiAllocator<ShaderString*> > uniform_list;
			
			shader = &_shader_programs[i];
			shader->capabilities = 0;
			shader->id = i;
			
			bool use_permutation = true;

			// add the base attributes and uniforms
			for( int a = 0; a < permutations.base.num_attributes; ++a )
			{
				attribute_list.push_back( &permutations.base.attributes[a] );
			}
			
			for( int a = 0; a < permutations.base.num_uniforms; ++a )
			{
				uniform_list.push_back( &permutations.base.uniforms[a] );
			}
			
			//LOGV( "----> permutation: %i\n", i );
			
			unsigned int requirements = 0;
			unsigned int conflicts = 0;
			
			// build preprocessor defines, attributes, and uniforms
			for( int p = 0; p < permutations.num_permutations; ++p )
			{
				if ( i & (1 << p) )
				{
					ShaderPermutationGroup * option = permutations.options[p];
					for( int id = 0; id < option->num_defines; ++id )
					{
						//LOGV( "option: %s\n", option->defines[id].c_str() );
						// preprocessor_defines.append( "#define " );
						// preprocessor_defines.append( option->defines[id].c_str() );
						// preprocessor_defines.append( " 1\n" );
						preprocessor_defines += "#define ";
						preprocessor_defines += option->defines[id].c_str();
						preprocessor_defines += " 1\n";
					}
					
					for( int a = 0; a < option->num_attributes; ++a )
					{
						attribute_list.push_back( &option->attributes[a] );
					}
					
					for( int a = 0; a < option->num_uniforms; ++a )
					{
						uniform_list.push_back( &option->uniforms[a] );
					}
					
					// build requirement list
					for( unsigned int i = 0; i < option->num_requires; ++i )
					{
						requirements |= find_parameter_mask( option->requires[ i ] );
					}

					// build conflict list
					for( unsigned int i = 0; i < option->num_conflicts; ++i )
					{
						conflicts |= find_parameter_mask( option->conflicts[ i ] );
					}

					shader->capabilities |= (1 << option->mask_value);
				}
			}
			
			
			
			if ( (requirements & shader->capabilities) < requirements )
			{
//				LOGV( "This shader will not compile properly. Missing one or more requirements!\n" );
				use_permutation = false;
			}
			else if ( (shader->capabilities & conflicts) > 0 )
			{
//				LOGV( "This shader will not compile properly. There are conflicting parameters\n" );
				use_permutation = false;
			}
						
			if ( !use_permutation )
			{
				//LOGV( "Skipping permutation: %i\n", shader->id );
				continue;
			}
			
			

			//LOGV( "%i -> %s\n", shader->id, preprocessor_defines.c_str() );
			renderer::ShaderObject vertex_shader = driver->shaderobject_create( renderer::SHADER_VERTEX );
			renderer::ShaderObject fragment_shader = driver->shaderobject_create( renderer::SHADER_FRAGMENT );

			// load the shaders and pass the defines
			if ( !driver->shaderobject_compile(vertex_shader, vs_source, preprocessor_defines.c_str(), shader_version() ) )
			{
				LOGW( "vertex_shader failed to compile!\n" );
				driver->shaderobject_destroy( vertex_shader );
				driver->shaderobject_destroy( fragment_shader );
				continue;
			}
			
			if ( !driver->shaderobject_compile(fragment_shader, fs_source, preprocessor_defines.c_str(), shader_version() ) )
			{
				LOGW( "fragment_shader failed to compile!\n" );
				driver->shaderobject_destroy( vertex_shader );
				driver->shaderobject_destroy( fragment_shader );
				continue;
			}

			// tally these up
			unsigned int total_attributes = attribute_list.size();
			unsigned int total_uniforms = uniform_list.size();
			
			// copy attributes
			//shader->attributes = CREATE_ARRAY( renderer::ShaderKeyValuePair, total_attributes );
			//shader->total_attributes = total_attributes;
			shader->alloc_attributes( total_attributes );
//			LOGV( "A shader->attributes: %p\n", shader->attributes );
			for( int attrib_id = 0; attrib_id < total_attributes; ++attrib_id )
			{
				ShaderKeyValuePair * kp = &shader->attributes[ attrib_id ];
				kp->set_key( (*attribute_list[ attrib_id ]).c_str() );
				kp->second = attrib_id;
			}
			
			// setup uniforms
			//shader->uniforms = CREATE_ARRAY( renderer::ShaderKeyValuePair, total_uniforms );
			//shader->total_uniforms = total_uniforms;
			shader->alloc_uniforms( total_uniforms );
//			LOGV( "A shader->uniforms: %p\n", shader->uniforms );
			for( int uniform_id = 0; uniform_id < total_uniforms; ++uniform_id )
			{
				ShaderKeyValuePair * kp = &shader->uniforms[ uniform_id ];
				kp->set_key( (*uniform_list[ uniform_id ]).c_str() );
				kp->second = -1;
			}

			shader->set_frag_data_location( shader_permutations().frag_location() );

			// attach compiled code to program
			driver->shaderprogram_attach( *shader, vertex_shader );
			driver->shaderprogram_attach( *shader, fragment_shader );

			// attributes must be bound before linking the program
			driver->shaderprogram_bind_attributes( *shader, *shader );

			// Link and validate the program; spit out any log info
			if ( driver->shaderprogram_link_and_validate( *shader, *shader ) )
			{
				driver->shaderprogram_activate( *shader );

				// loop through all uniforms and cache their locations
				driver->shaderprogram_bind_uniforms( *shader, *shader );
				
				driver->shaderprogram_deactivate( *shader );
			}
			else
			{
				LOGW( "shader program link and validate FAILED\n" );
			}
			
			driver->shaderprogram_detach( *shader, vertex_shader );
			driver->shaderprogram_detach( *shader, fragment_shader );

			// clean up
			driver->shaderobject_destroy( vertex_shader );
			driver->shaderobject_destroy( fragment_shader );

			attribute_list.clear();
			uniform_list.clear();			
		}

		// cleanup resources
		DEALLOC(vs_source);
		DEALLOC(fs_source);
		
		shader_program.object = 0;
		driver->shaderprogram_deactivate( shader_program );
		
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
	} // find_compatible_shader


	renderer::ShaderObject create_shader_from_file( const char * shader_path, renderer::ShaderObjectType type, const char * preprocessor_defines )
	{
		renderer::ShaderObject shader_object;
		char * buffer;
		int length = 0;
		buffer = fs::file_to_buffer( shader_path, 0, &length );
		if ( buffer )
		{
			StackString<32> version;
			util::strip_shader_version( buffer, version );
			if ( version._length == 0 )
			{
#if defined(PLATFORM_IS_MOBILE) || defined(PLATFORM_IS_RASPBERRYPI)
				version = "#version 100";
#else
				version = "#version 150";
#endif
				LOGW( "Unable to extract version from shader! Forcing to '%s'.\n", version() );
				
			}
			version.append( "\n" );
			
			// specify version string first, followed by any defines, then the actual shader source
			if ( preprocessor_defines == 0 )
			{
				preprocessor_defines = "";
			}
			
			shader_object = renderer::driver()->shaderobject_create( type );
			
			if ( !renderer::driver()->shaderobject_compile( shader_object, buffer, preprocessor_defines, version()) )
			{
				LOGE( "Error compiling shader %s\n", shader_path );
			}
			
			DEALLOC(buffer);
		}
		else
		{
			LOGE( "Unable to open shader '%s'\n", shader_path );
		}
		
		return shader_object;
	}
	
	void load_shader( const char * shader_path, Shader * shader )
	{
		LOGV( "loading shader '%s'\n", shader_path );
		StackString<MAX_PATH_SIZE> filename = shader_path;
		renderer::ShaderParameters params;
		
		renderer::ShaderProgram shader_program;
		shader_program.object = 0;
		if (!renderer::driver())
		{
			LOGW( "Renderer is not initialized!\n" );
			return;
		}
		renderer::driver()->shaderprogram_deactivate( shader_program );
		
		renderer::ShaderProgram program = renderer::driver()->shaderprogram_create( params );
		shader->object = program.object;
		
		filename.append( ".vert" );
		renderer::ShaderObject vertex_shader = create_shader_from_file( filename(), renderer::SHADER_VERTEX, 0 );
		
		filename = shader_path;
		filename.append( ".frag" );
		renderer::ShaderObject fragment_shader = create_shader_from_file( filename(), renderer::SHADER_FRAGMENT, 0 );
		
		renderer::driver()->shaderprogram_attach( *shader, vertex_shader );
		renderer::driver()->shaderprogram_attach( *shader, fragment_shader );
	
		renderer::driver()->shaderprogram_bind_attributes( *shader, *shader );
		if ( renderer::driver()->shaderprogram_link_and_validate( *shader, *shader ) )
		{
			renderer::driver()->shaderprogram_activate( *shader );
			renderer::driver()->shaderprogram_bind_uniforms( *shader, *shader );
			renderer::driver()->shaderprogram_deactivate( *shader );
		}
		
		renderer::driver()->shaderprogram_detach( *shader, vertex_shader );
		renderer::driver()->shaderprogram_detach( *shader, fragment_shader );
		
		renderer::driver()->shaderobject_destroy( vertex_shader );
		renderer::driver()->shaderobject_destroy( fragment_shader );
	} // load_shader
	
	void destroy_shader( Shader * shader )
	{
		renderer::ShaderProgram program;
		program.object = shader->object;
		renderer::driver()->shaderprogram_destroy( program );
	} // destroy_shader

	void load_test_shader( Shader * shader )
	{
		renderer::ShaderParameters params;

		renderer::ShaderProgram program = renderer::driver()->shaderprogram_create( params );
		shader->object = program.object;
	
		renderer::ShaderObject vertex_shader = create_shader_from_file( "shaders/fontshader.vert", renderer::SHADER_VERTEX, 0 );
		renderer::ShaderObject fragment_shader = create_shader_from_file( "shaders/fontshader.frag", renderer::SHADER_FRAGMENT, 0 );
		
		renderer::driver()->shaderprogram_attach( *shader, vertex_shader );
		renderer::driver()->shaderprogram_attach( *shader, fragment_shader );
		
		
		shader->set_frag_data_location( "out_color" );
		shader->alloc_uniforms( 3 );
		shader->uniforms[0].set_key( "projection_matrix" );
		shader->uniforms[1].set_key( "modelview_matrix" );
		shader->uniforms[2].set_key( "diffusemap" );
		
		shader->alloc_attributes( 3 );
		shader->attributes[0].set_key( "in_position" ); shader->attributes[0].second = 0;
		shader->attributes[1].set_key( "in_color" ); shader->attributes[1].second = 1;
		shader->attributes[2].set_key( "in_uv" ); shader->attributes[2].second = 2;

		
		renderer::driver()->shaderprogram_bind_attributes( *shader, *shader );
		renderer::driver()->shaderprogram_link_and_validate( *shader, *shader );
		
		renderer::driver()->shaderprogram_bind_uniforms( *shader, *shader );
		
		renderer::driver()->shaderobject_destroy( vertex_shader );
		renderer::driver()->shaderobject_destroy( fragment_shader );
		
	} // load_test_shader


	unsigned int find_parameter_mask( ShaderString & name )
	{
		// TODO: need to validate the name here against the
		// parameter names in permutations config file.
		
		
		if ( _shader_permutations != 0 )
		{
			for( unsigned int option_id = 0; option_id < shader_permutations().num_permutations; ++option_id )
			{
				ShaderPermutationGroup * option = shader_permutations().options[ option_id ];
				if ( xstr_nicmp( (const char*)name.c_str(), option->name.c_str(), 64 ) == 0 )
				{
					//				LOGV( "mask for %s is %i\n", name.c_str(), option->mask_value );
					return (1 << option->mask_value);
				}
			}
		}
		return 0;
	} // find_parameter_mask
}; // namespace assets


namespace assets
{
	
	
#define DECLARE_ASSET_LIBRARY( type, name )\
	type * _##name = 0;\
	type * name()\
	{\
		return _##name;\
	}
	
	
	TextureAssetLibrary * _textures = 0;
	TextureAssetLibrary * textures()
	{
		return _textures;
	}
	
	Texture * _default_texture = 0;
	
	MeshAssetLibrary * _meshes = 0;
	MeshAssetLibrary * meshes()
	{
		return _meshes;
	}
	
	
	MaterialAssetLibrary * _materials = 0;
	MaterialAssetLibrary * materials()
	{
		return _materials;
	}
	
	Material * _default_material = 0;
	
	void load_default_texture_and_material()
	{
		// setup default texture
		_default_texture = textures()->allocate_asset();
		_default_texture->texture_id = image::load_default_texture();
		textures()->take_ownership("textures/default", _default_texture);
		LOGV( "Loaded default texture; id = %i, asset_id = %i\n", _default_texture->texture_id, _default_texture->asset_id );
		
		// setup default material
		_default_material = materials()->allocate_asset();
		_default_material->name = "default";
		_default_material->num_parameters = 1;
		Material::Parameter * parameter = _default_material->parameters = CREATE(Material::Parameter);
		parameter->name = "diffusemap";
		parameter->type = MP_SAMPLER_2D;
		parameter->texture_unit = texture_unit_for_map( parameter->name );
		parameter->intValue = _default_texture->Id();
		_default_material->calculate_requirements();
		materials()->take_ownership( "materials/default", _default_material );
		LOGV( "Loaded default materials; asset_id = %i\n", _default_material->asset_id );
		
	} // load_default_texture_and_material


	void startup()
	{
		// allocate asset libraries
		_textures = CREATE(TextureAssetLibrary, texture_load_callback, texture_construct_extension);
		_meshes = CREATE(MeshAssetLibrary, mesh_load_callback, mesh_construct_extension);
		_materials = CREATE(MaterialAssetLibrary, material_load_callback, material_construct_extension);

		// load shader permutations
		compile_shader_permutations();

		load_default_texture_and_material();
	} // startup
	
	void purge()
	{
	
		for( unsigned int shader_id = 0; shader_id < total_shaders; ++shader_id )
		{
			ShaderParameters params;
			renderer::driver()->shaderprogram_destroy( _shader_programs[ shader_id ] );
		}
	
		DESTROY_ARRAY( Shader, _shader_programs, total_shaders );
		DESTROY( ShaderPermutations, _shader_permutations );
		
		if ( _textures )
		{
			_textures->release_and_purge();
		}
		
		if ( _meshes )
		{
			_meshes->release_and_purge();
		}
		
		if ( _materials )
		{
			_materials->release_and_purge();
		}
		
	} // purge
	
	void shutdown()
	{
		purge();
		DESTROY(TextureAssetLibrary, _textures);
		DESTROY(MeshAssetLibrary, _meshes);
		DESTROY(MaterialAssetLibrary, _materials);
		
		_default_material = 0;
		_default_texture = 0;
	} // shutdown

	void append_asset_extension( AssetType type, StackString<MAX_PATH_SIZE> & path )
	{
		const char * extension = "";
		kernel::KernelDeviceFlags device_flags = kernel::instance()->parameters().device_flags;
		
		switch( type )
		{
			case SoundAsset:
			{
#if __APPLE__ && PLATFORM_IS_MOBILE
				if ( (device_flags & kernel::DeviceiPad) || (device_flags & kernel::DeviceiPhone) )
				{
					extension = "caf";
				}
#else
				extension = "ogg";
#endif
				break;
			} // SoundAsset
			
//			case TextureAsset:
//			{
//				if ( device_flags & kernel::DeviceDesktop || (device_flags & kernel::DeviceAndroid) )
//				{
//					// ...
//					extension = "png";
//				}
//				else if ( device_flags & kernel::DeviceiPhone )
//				{
//					extension = "png";
//					path.append( "-iphone" );
//				}
//				else if ( device_flags & kernel::DeviceiPad )
//				{
//					extension = "png";
//					path.append( "-ipad" );
//				}
//				
//				if ( device_flags & kernel::DeviceSupportsRetinaDisplay )
//				{
//					path.append( "@2x" );
//				}
//				
//				break;
//			} // TextureAsset
//			
//			
//			case MaterialAsset:
//			{
//				extension = "material";
//				break;
//			} // MaterialAsset
//			
//			case MeshAsset:
//			{
//				extension = "model";
//				break;
//			} // MeshAsset
			
			default: LOGW( "AssetType %i is NOT supported!\n" ); break;
		}
		
		path.append( "." );
		path.append( extension );
		
	} // append_asset_extension
		
}; // namespace assets
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
#include <gemini/util/stackstring.h>
#include <gemini/core/filesystem.h>
#include <gemini/util/configloader.h>

#include "assets.h"
#include "renderer/renderer.h"
#include "assets/asset_shader.h"


namespace assets
{
	namespace _internal
	{
		Json::Value shader_config;
	};
	
	// -------------------------------------------------------------
	// Shader
	
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
	
	void Shader::release()
	{
		renderer::ShaderProgram program;
		program.object = object;

		renderer::driver()->shaderprogram_destroy( program );
	}
	
	void Shader::show_uniforms()
	{
		LOGV("uniforms:\n");
		for (size_t i = 0; i < total_uniforms; ++i)
		{
			LOGV("%s -> %i\n", uniforms[i].first, uniforms[i].second);
		}
	}
	
	void Shader::show_attributes()
	{
		LOGV("attributes:\n");
		for (size_t i = 0; i < total_attributes; ++i)
		{
			LOGV("%s -> %i\n", attributes[i].first, attributes[i].second);
		}
	}
		
	renderer::ShaderObject create_shader_from_file( const char * shader_path, renderer::ShaderObjectType type, const char * preprocessor_defines )
	{
		renderer::ShaderObject shader_object;
		char * buffer;
		size_t length = 0;
		buffer = core::filesystem::file_to_buffer( shader_path, 0, &length );
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
	} // create_shader_from_file
//	
//	void load_shader( const char * shader_path, Shader * shader )
//	{
//		LOGV( "loading shader '%s'\n", shader_path );
//		StackString<MAX_PATH_SIZE> filename = shader_path;
//		renderer::ShaderParameters params;
//		
//		renderer::ShaderProgram shader_program;
//		shader_program.object = 0;
//		if (!renderer::driver())
//		{
//			LOGW( "Renderer is not initialized!\n" );
//			return;
//		}
//		renderer::driver()->shaderprogram_deactivate( shader_program );
//		
//		renderer::ShaderProgram program = renderer::driver()->shaderprogram_create( params );
//		shader->object = program.object;
//		
//		filename.append( ".vert" );
//		renderer::ShaderObject vertex_shader = create_shader_from_file( filename(), renderer::SHADER_VERTEX, 0 );
//		
//		filename = shader_path;
//		filename.append( ".frag" );
//		renderer::ShaderObject fragment_shader = create_shader_from_file( filename(), renderer::SHADER_FRAGMENT, 0 );
//		
//		renderer::driver()->shaderprogram_attach( *shader, vertex_shader );
//		renderer::driver()->shaderprogram_attach( *shader, fragment_shader );
//		
//		renderer::driver()->shaderprogram_bind_attributes( *shader, *shader );
//		if ( renderer::driver()->shaderprogram_link_and_validate( *shader, *shader ) )
//		{
//			renderer::driver()->shaderprogram_activate( *shader );
//			renderer::driver()->shaderprogram_bind_uniforms( *shader, *shader );
//			renderer::driver()->shaderprogram_deactivate( *shader );
//		}
//		
//		renderer::driver()->shaderprogram_detach( *shader, vertex_shader );
//		renderer::driver()->shaderprogram_detach( *shader, fragment_shader );
//		
//		renderer::driver()->shaderobject_destroy( vertex_shader );
//		renderer::driver()->shaderobject_destroy( fragment_shader );
//	} // load_shader
	
//	void destroy_shader( Shader * shader )
//	{
//		renderer::ShaderProgram program;
//		program.object = shader->object;
//		renderer::driver()->shaderprogram_destroy( program );
//	} // destroy_shader
	
//	void load_test_shader( Shader * shader )
//	{
//		renderer::ShaderParameters params;
//		
//		renderer::ShaderProgram program = renderer::driver()->shaderprogram_create( params );
//		shader->object = program.object;
//		
//		renderer::ShaderObject vertex_shader = create_shader_from_file( "shaders/fontshader.vert", renderer::SHADER_VERTEX, 0 );
//		renderer::ShaderObject fragment_shader = create_shader_from_file( "shaders/fontshader.frag", renderer::SHADER_FRAGMENT, 0 );
//		
//		renderer::driver()->shaderprogram_attach( *shader, vertex_shader );
//		renderer::driver()->shaderprogram_attach( *shader, fragment_shader );
//		
//		
//		shader->set_frag_data_location( "out_color" );
//		shader->alloc_uniforms( 3 );
//		shader->uniforms[0].set_key( "projection_matrix" );
//		shader->uniforms[1].set_key( "modelview_matrix" );
//		shader->uniforms[2].set_key( "diffusemap" );
//		
//		shader->alloc_attributes( 3 );
//		shader->attributes[0].set_key( "in_position" ); shader->attributes[0].second = 0;
//		shader->attributes[1].set_key( "in_color" ); shader->attributes[1].second = 1;
//		shader->attributes[2].set_key( "in_uv" ); shader->attributes[2].second = 2;
//		
//		
//		renderer::driver()->shaderprogram_bind_attributes( *shader, *shader );
//		renderer::driver()->shaderprogram_link_and_validate( *shader, *shader );
//		
//		renderer::driver()->shaderprogram_bind_uniforms( *shader, *shader );
//		
//		renderer::driver()->shaderobject_destroy( vertex_shader );
//		renderer::driver()->shaderobject_destroy( fragment_shader );
//		
//	} // load_test_shader

	typedef std::vector< std::string, GeminiAllocator<std::string> > StringVector;
	void append_list_items(StringVector& vec, const Json::Value& array)
	{
		if (array.isNull())
		{
			return;
		}
		
		Json::ValueIterator iter = array.begin();
		for ( ; iter != array.end(); ++iter)
		{
			Json::Value& value = *iter;
			vec.push_back(value.asString());
		}
	}
	
	void create_shader_attributes_and_uniforms(Shader* shader, StringVector& attributes, StringVector& uniforms)
	{
		shader->alloc_attributes(attributes.size());
		int index = 0;
		for (auto value : attributes)
		{
			shader->attributes[index].set_key(value.c_str());
			shader->attributes[index].second = index;
			++index;
		}
		
		shader->alloc_uniforms(uniforms.size());
		index = 0;
		for (auto value : uniforms)
		{
			shader->uniforms[index].set_key(value.c_str());
			shader->uniforms[index].second = index;
			++index;
		}
	}
	
	bool create_shader_program_from_file(Shader* shader, const char* path)
	{
		if (!renderer::driver())
		{
			LOGW( "Renderer is not initialized!\n" );
			return false;
		}
		
		renderer::IRenderDriver* driver = renderer::driver();
		StackString<MAX_PATH_SIZE> filename = path;
		
		// see if the required files exist
		filename.append(".vert");
		if (!core::filesystem::file_exists(filename()))
		{
			LOGV("vertex shader does not exist: %s\n", filename());
			return false;
		}
		
		filename = path;
		filename.append(".frag");
		if (!core::filesystem::file_exists(filename()))
		{
			LOGV("fragment shader does not exist: %s\n", filename());
			return false;
		}
		
		renderer::ShaderProgram shader_program;
		shader_program.object = 0;
		driver->shaderprogram_deactivate(shader_program);

		renderer::ShaderParameters params;
		renderer::ShaderProgram program = driver->shaderprogram_create(params);
		shader->object = program.object;
		
		filename = path;
		filename.append(".vert");
		renderer::ShaderObject vertex_shader = create_shader_from_file(filename(), renderer::SHADER_VERTEX, 0);
		
		filename = path;
		filename.append(".frag");
		renderer::ShaderObject fragment_shader = create_shader_from_file(filename(), renderer::SHADER_FRAGMENT, 0);
		
		driver->shaderprogram_attach(*shader, vertex_shader);
		driver->shaderprogram_attach(*shader, fragment_shader);
		
		driver->shaderprogram_bind_attributes(*shader, *shader);
		if (driver->shaderprogram_link_and_validate(*shader, *shader))
		{
			driver->shaderprogram_activate(*shader);
			driver->shaderprogram_bind_uniforms(*shader, *shader);
			driver->shaderprogram_deactivate(*shader);
		}
		
		driver->shaderprogram_detach(*shader, vertex_shader);
		driver->shaderprogram_detach(*shader, fragment_shader);
		
		driver->shaderobject_destroy(vertex_shader);
		driver->shaderobject_destroy(fragment_shader);
	
		return true;
	}

	AssetLoadStatus shader_load_callback(const char* path, Shader* shader, const AssetParameters& parameters)
	{
		// populate all attribute names
		const Json::Value& attribute_block = _internal::shader_config["attribute_block"];
		StringVector attributes;
		append_list_items(attributes, attribute_block);

		// populate all uniform names
		const Json::Value& uniform_block = _internal::shader_config["uniform_block"];
		StringVector uniforms;
		append_list_items(uniforms, uniform_block);
		
		// find the specific shader requested in the shader config
		StackString<128> shader_path = path;
		StackString<128> shader_name = shader_path.basename();
		// We could actually use the dirname here in case someone requests
		// a shader that doesn't reside in "shaders". Nah, unlikely.
		
		const Json::Value& shader_list = _internal::shader_config["shaders"];
		LOGV("requested shader is: %s\n", shader_name());
		
		const Json::Value& shader_block = shader_list[ shader_name() ];
		if (shader_block.isNull())
		{
			LOGV("unable to find the shader block named \"%s\"\n", shader_name());
			return AssetLoad_Failure;
		}
		
		append_list_items(attributes, shader_block["attributes"]);
		append_list_items(uniforms, shader_block["uniforms"]);
		
		shader->set_frag_data_location("out_color");
		create_shader_attributes_and_uniforms(shader, attributes, uniforms);

		bool success = create_shader_program_from_file(shader, path);
		if (!success)
		{
			return AssetLoad_Failure;
		}

		return AssetLoad_Success;
	} // font_load_callback
	
	void shader_construct_extension(StackString<MAX_PATH_SIZE>& extension)
	{
	} // shader_construct_extension
	
	
	util::ConfigLoadStatus shader_parameter_load_callback(const Json::Value& root, void* context)
	{
		Json::Value* shader_parameters = static_cast<Json::Value*>(context);
		
		*shader_parameters = root;
				
		return util::ConfigLoad_Success;
	}

	void create_shader_config()
	{
		bool status = util::json_load_with_callback(SHADER_CONFIG, shader_parameter_load_callback, &_internal::shader_config, true);
		if (!status)
		{
			LOGW("Error loading shader parameters!\n");
		}
	} // create_shader_config
	
	void destroy_shader_config()
	{
		_internal::shader_config.clear();
	} // destroy_shader_config
	
}; // namespace assets
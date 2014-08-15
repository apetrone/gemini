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
		
	renderer::ShaderObject create_shader_from_file( const char * shader_path, renderer::ShaderObjectType type, std::string& preprocessor_defines )
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
				//LOGW( "Unable to extract version from shader! Forcing to '%s'.\n", version() );
			}
			version.append( "\n" );
			
			// specify version string first, followed by any defines, then the actual shader source
			if ( preprocessor_defines.empty() )
			{
				preprocessor_defines = "";
			}
			
			shader_object = renderer::driver()->shaderobject_create( type );
			
			if ( !renderer::driver()->shaderobject_compile( shader_object, buffer, preprocessor_defines.c_str(), version()) )
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
		if (shader->total_attributes == 0)
		{
			shader->alloc_attributes(attributes.size());
			int index = 0;
			for (auto value : attributes)
			{
				shader->attributes[index].set_key(value.c_str());
				shader->attributes[index].second = index;
				++index;
			}
		}
		
		if (shader->total_uniforms == 0)
		{
			shader->alloc_uniforms(uniforms.size());
			int index = 0;
			for (auto value : uniforms)
			{
				shader->uniforms[index].set_key(value.c_str());
				shader->uniforms[index].second = index;
				++index;
			}
		}
	}
	
	bool verify_stages_exist_on_disk(const char* path, StringVector& stages)
	{
		for (auto name : stages)
		{
			StackString<MAX_PATH_SIZE> filename = path;
			filename.append(".");
			filename.append(name.c_str());
			
			// verify the file exists
			if (!core::filesystem::file_exists(filename()))
			{
				LOGE("\"%s\" does not exist!\n", filename());
				return false;
			}
		}

		return true;
	}
	
	const std::string shader_stage_to_extension(const std::string& extension)
	{
		return std::string(".") + extension;
	}
	
	renderer::ShaderObjectType shader_stage_to_shaderobject_type(const std::string& stage)
	{
		if (stage == "vert")
		{
			return renderer::SHADER_VERTEX;
		}
		else if (stage == "frag")
		{
			return renderer::SHADER_FRAGMENT;
		}
		else if (stage == "geom")
		{
			return renderer::SHADER_GEOMETRY;
		}
		else if (stage == "comp")
		{
			return renderer::SHADER_COMPUTE;
		}
		else
		{
			LOGE("Unknown shader stage: %s!\n", stage.c_str());
		}
	}
	
	bool create_shader_program_from_file(Shader* shader, const char* path, StringVector& stages, std::string& preprocessor)
	{
		if (!renderer::driver())
		{
			LOGW( "Renderer is not initialized!\n" );
			return false;
		}

		renderer::IRenderDriver* driver = renderer::driver();
		
		// verify all stages exist
		bool stages_exist = verify_stages_exist_on_disk(path, stages);
		if (!stages_exist)
		{
			LOGW("One or more stages do not exist for shader %s. Aborting.\n", path);
			return false;
		}

		// shader program is uninitialized; create a new one
		if (shader->object == 0)
		{
			renderer::ShaderParameters params;
			renderer::ShaderProgram program = driver->shaderprogram_create(params);
			shader->object = program.object;
		}

		// attach shader objects to program
		FixedArray<renderer::ShaderObject> shader_objects;
		shader_objects.allocate(stages.size());
		for (int i = 0; i < shader_objects.size(); ++i)
		{
			std::string filename = path + shader_stage_to_extension(stages[i]);
			shader_objects[i] = create_shader_from_file(filename.c_str(), shader_stage_to_shaderobject_type(stages[i]), preprocessor);
			driver->shaderprogram_attach(*shader, shader_objects[i]);
		}

		// attributes must be bound before linking the program
		driver->shaderprogram_bind_attributes(*shader, *shader);
		
		// Link and validate the program; spit out any log info
		if (driver->shaderprogram_link_and_validate(*shader, *shader))
		{
			driver->shaderprogram_activate(*shader);
			
			// loop through all uniforms and cache their locations
			driver->shaderprogram_bind_uniforms(*shader, *shader);
			
			driver->shaderprogram_deactivate(*shader);
		}

		// detach and destroy shader objects
		for (auto& object : shader_objects)
		{
			driver->shaderprogram_detach(*shader, object);
			driver->shaderobject_destroy(object);
		}

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
		
		const Json::Value& shader_block = shader_list[ shader_name() ];
		if (shader_block.isNull())
		{
			LOGV("unable to find the shader block named \"%s\"\n", shader_name());
			return AssetLoad_Failure;
		}
		
		append_list_items(attributes, shader_block["attributes"]);
		append_list_items(uniforms, shader_block["uniforms"]);
		
		StringVector stages;
		append_list_items(stages, shader_block["stages"]);


		StringVector preprocessor;
		Json::Value preprocessor_block = _internal::shader_config["preprocessor_block"];
		append_list_items(preprocessor, preprocessor_block);

		shader->set_frag_data_location("out_color");
		create_shader_attributes_and_uniforms(shader, attributes, uniforms);

		std::string preprocessor_defines;
		for (auto item : preprocessor)
		{
			preprocessor_defines += item;
			preprocessor_defines += "\n";
		}
		
		bool success = create_shader_program_from_file(shader, path, stages, preprocessor_defines);
		if (!success)
		{
			return AssetLoad_Failure;
		}
		
		
//		shader->show_attributes();

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
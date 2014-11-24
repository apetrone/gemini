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
#include "shaderconfig.h"
#include "render_utilities.h"

#include <core/filesystem.h>
#include <core/configloader.h>

#include <slim/xlog.h>

#include <string>

namespace renderer
{
	namespace shader_config
	{
		const char _SHADER_CONFIG[] = "conf/shaders.conf";
			
		Json::Value _shader_config;
		
		typedef std::vector< std::string, GeminiAllocator<std::string> > StringVector;

		util::ConfigLoadStatus shader_parameter_load_callback(const Json::Value& root, void* context)
		{
			Json::Value* shader_parameters = static_cast<Json::Value*>(context);
			
			*shader_parameters = root;
			
			return util::ConfigLoad_Success;
		}
		
		void startup()
		{
			bool status = util::json_load_with_callback(_SHADER_CONFIG, shader_parameter_load_callback, &_shader_config, true);
			if (!status)
			{
				LOGW("Error loading shader parameters!\n");
			}
		} // startup
		
		void shutdown()
		{
			_shader_config.clear();
		} // shutdown


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
		
		bool fetch_shader_config(
			const Json::Value& shader_config,
			const char* shader_name,
			StringVector& attributes,
			StringVector& uniforms,
			StringVector& stages,
			StringVector& preprocessor)
		{
			// populate all attribute names
			const Json::Value& attribute_block = shader_config["attribute_block"];
			append_list_items(attributes, attribute_block);
			
			// populate all uniform names
			const Json::Value& uniform_block = shader_config["uniform_block"];
			append_list_items(uniforms, uniform_block);
			
			const Json::Value& shader_list = shader_config["shaders"];
			
			const Json::Value& shader_block = shader_list[ shader_name ];
			if (shader_block.isNull())
			{
				LOGV("Unable to find the shader block named \"%s\"\n", shader_name);
				return false;
			}
			
			append_list_items(attributes, shader_block["attributes"]);
			append_list_items(uniforms, shader_block["uniforms"]);
			append_list_items(stages, shader_block["stages"]);

			Json::Value preprocessor_block = shader_config["preprocessor_block"];
			append_list_items(preprocessor, preprocessor_block);
			
			return true;
		}
		
		
		void map_attributes_and_uniforms(renderer::ShaderProgram* shader, StringVector& attributes, StringVector& uniforms)
		{
			if (shader->attributes.empty())
			{
				shader->attributes.allocate(attributes.size());
				int index = 0;
				for (auto value : attributes)
				{
					shader->attributes[index].first = value;
					shader->attributes[index].second = index;
					++index;
				}
			}
			
			if (shader->uniforms.empty())
			{
				shader->uniforms.allocate(uniforms.size());
				int index = 0;
				for (auto value : uniforms)
				{
					shader->uniforms[index].first = value;
					shader->uniforms[index].second = index;
					++index;
				}
			}
		} // map_attributes_and_uniforms
		
		
		bool verify_stages_exist_on_disk(const char* path, const StringVector& stages)
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
				return renderer::SHADER_VERTEX;
			}
		}
		
	#if 0
		bool create_shader_program_from_file(const char* path, StringVector& stages, std::string& preprocessor)
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
			if (shader == 0)
			{
				shader = driver->shaderprogram_create();
			}
			
			// attach shader objects to program
			FixedArray<renderer::ShaderObject> shader_objects;
			shader_objects.allocate(stages.size());
			for (int i = 0; i < shader_objects.size(); ++i)
			{
				std::string filename = path + shader_stage_to_extension(stages[i]);
				shader_objects[i] = create_shader_from_file(filename.c_str(), shader_stage_to_shaderobject_type(stages[i]), preprocessor);
				driver->shaderprogram_attach(shader, shader_objects[i]);
			}
			
			// attributes must be bound before linking the program
			driver->shaderprogram_bind_attributes(shader);
			
			// Link and validate the program; spit out any log info
			if (driver->shaderprogram_link_and_validate(shader))
			{
				driver->shaderprogram_activate(shader);
				
				// loop through all uniforms and cache their locations
				driver->shaderprogram_bind_uniforms(shader);
				
	//			driver->shaderprogram_bind_uniform_block(*shader, *shader, "constant_buffer");
				
				driver->shaderprogram_deactivate(shader);
			}
			
			// detach and destroy shader objects
			for (auto& object : shader_objects)
			{
				driver->shaderprogram_detach(*shader, object);
				driver->shaderobject_destroy(object);
			}
			
			return true;
		}
	#endif
		
		
		renderer::ShaderObject create_shader_from_file(const char* shader_path, renderer::ShaderObjectType type, std::string& preprocessor_defines )
		{
			renderer::ShaderObject shader_object;
			char* buffer;
			size_t length = 0;
			buffer = core::filesystem::file_to_buffer( shader_path, 0, &length );
			if ( buffer )
			{
				StackString<32> version;
				render_utilities::strip_shader_version( buffer, version );
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
					LOGE( "Error compiling shader!\n" );
				}
				
				DEALLOC(buffer);
			}
			
			return shader_object;
		} // create_shader_from_file
		
		bool setup_program_with_config(renderer::IRenderDriver* driver, renderer::ShaderProgram* program, const char* path, const StringVector& stages, std::string& preprocessor)
		{
			// verify all stages exist
			bool stages_exist = verify_stages_exist_on_disk(path, stages);
			if (!stages_exist)
			{
				LOGW("One or more stages do not exist for shader %s. Aborting.\n", path);
				return false;
			}
			
			// attach shader objects to program
			FixedArray<renderer::ShaderObject> shader_objects;
			shader_objects.allocate(stages.size());
			for (int i = 0; i < shader_objects.size(); ++i)
			{
				std::string filename = path + shader_stage_to_extension(stages[i]);
				shader_objects[i] = create_shader_from_file(filename.c_str(), shader_stage_to_shaderobject_type(stages[i]), preprocessor);
				driver->shaderprogram_attach(program, shader_objects[i]);
			}
			
			// attributes must be bound before linking the program
			driver->shaderprogram_bind_attributes(program);
			
			// Link and validate the program; spit out any log info
			if (driver->shaderprogram_link_and_validate(program))
			{
				driver->shaderprogram_activate(program);
				
				// loop through all uniforms and cache their locations
				driver->shaderprogram_bind_uniforms(program);
				
	//			driver->shaderprogram_bind_uniform_block(*shader, *shader, "constant_buffer");
				
				driver->shaderprogram_deactivate(program);
			}
			
			// detach and destroy shader objects
			for (auto& object : shader_objects)
			{
				driver->shaderprogram_detach(program, object);
				driver->shaderobject_destroy(object);
			}
			
			return true;
		}
		
		renderer::ShaderProgram* load_shaderprogram_from_file(const char* path)
		{
			// find the specific shader requested in the shader config
			// We could actually use the dirname here in case someone requests
			// a shader that doesn't reside in "shaders". Nah, unlikely.
			StackString<128> shader_path = path;
			StackString<128> shader_name = shader_path.basename();
			
			// look in the shader config json to find a shader that matches
			// load all attributes, uniforms, stages, and preprocessor defines we'll need.
			StringVector attributes, uniforms, stages, preprocessor;
			if (!fetch_shader_config(_shader_config, shader_name(), attributes, uniforms, stages, preprocessor))
			{
				LOGE("Error while loading shader \"%s\" from shader_config\n", shader_name());
				return nullptr;
			}
			
			// create a single string of all the preprocessor_defines
			std::string preprocessor_defines;
			for (auto item : preprocessor)
			{
				preprocessor_defines += item;
				preprocessor_defines += "\n";
			}
			
			renderer::IRenderDriver* driver = renderer::driver();
			renderer::ShaderProgram* program = driver->shaderprogram_create();
			
			assert(program != 0);
			map_attributes_and_uniforms(program, attributes, uniforms);
			
			bool result = setup_program_with_config(driver, program, path, stages, preprocessor_defines);
			if (!result)
			{
				LOGE("Error while creating shader \"%s\"\n", path);
				driver->shaderprogram_destroy(program);
				program = nullptr;
			}
			
			return program;
		}
	}
}; // namespace renderer
// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include "shaderconfig.h"
#include "render_utilities.h"

#include <core/filesystem.h>
#include <core/configloader.h>
#include <core/logging.h>

#include <string>

using namespace core;

namespace gemini
{
	namespace renderer
	{
		namespace shader_config
		{
			const char _SHADER_CONFIG[] = "conf/shaders.conf";
				
			Json::Value _shader_config;
			
			typedef std::vector< std::string, CustomPlatformAllocator<std::string> > StringVector;

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
				core::FixedArray<renderer::ShaderObject> shader_objects;
				shader_objects.allocate(stages.size());
				for (size_t i = 0; i < shader_objects.size(); ++i)
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
			
			void load_shaderprogram_from_file(const char* path, renderer::ShaderProgram** shader_program)
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
					return;
				}
				
				// create a single string of all the preprocessor_defines
				std::string preprocessor_defines;
				for (auto item : preprocessor)
				{
					preprocessor_defines += item;
					preprocessor_defines += "\n";
				}
				

				assert(shader_program != 0);
				renderer::IRenderDriver* driver = renderer::driver();
				renderer::ShaderProgram* program = *shader_program;
				if (!program)
				{
					program = driver->shaderprogram_create();
				}

				assert(program != 0);
				map_attributes_and_uniforms(program, attributes, uniforms);
				
				bool result = setup_program_with_config(driver, program, path, stages, preprocessor_defines);
				if (!result)
				{
					LOGE("Error while creating shader \"%s\"\n", path);
					driver->shaderprogram_destroy(program);
					program = nullptr;
				}
				
				*shader_program = program;
			}
		}
	} // namespace renderer
} // namespace gemini
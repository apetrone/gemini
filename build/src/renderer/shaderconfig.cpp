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

#include <core/logging.h>

#include <string>

using namespace core;

namespace renderer
{
	namespace shader_config
	{
		struct ShaderDescriptionBlock
		{
			core::StackString<32> name;
			Array<core::StackString<32>> stages;
			Array<core::StackString<32>> attributes;
			Array<core::StackString<32>> uniforms;

			ShaderDescriptionBlock(gemini::Allocator& allocator, const char* shader_name = nullptr)
				: name(shader_name)
				, stages(allocator)
				, attributes(allocator)
				, uniforms(allocator)
			{
			}
		};

		struct ShaderConfiguration
		{
			Array<core::StackString<64>> preprocessor_defines;
			HashSet<core::StackString<32>, ShaderDescriptionBlock> shaders;

			ShaderConfiguration(gemini::Allocator& allocator)
				: preprocessor_defines(allocator)
				, shaders(allocator, HASHSET_INITIAL_SIZE, HASHSET_GROWTH_FACTOR, ShaderDescriptionBlock(allocator))
			{
			}
		};

		ShaderConfiguration* _shader_config = nullptr;
		typedef std::vector<std::string> StringVector;

		void add_shader(ShaderConfiguration* configuration, const ShaderDescriptionBlock& shader)
		{
			configuration->shaders[shader.name] = shader;
		}

		void startup(gemini::Allocator& allocator)
		{
			// This data is hard-coded here until this renderer is removed
			// as it's not worth spending much time on throw-away code.
			_shader_config = MEMORY_NEW(ShaderConfiguration, core::memory::global_allocator())(allocator);

			ShaderDescriptionBlock objects(allocator, "objects");
			objects.stages.push_back("vert");
			objects.stages.push_back("frag");
			objects.attributes.push_back("in_position");
			objects.attributes.push_back("in_normal");
			objects.attributes.push_back("in_uv0");
			objects.attributes.push_back("in_uv1");
			objects.uniforms.push_back("modelview_matrix");
			objects.uniforms.push_back("projection_matrix");
			objects.uniforms.push_back("object_matrix");
			objects.uniforms.push_back("diffusemap");
			objects.uniforms.push_back("lightmap");
			objects.uniforms.push_back("viewer_direction");
			objects.uniforms.push_back("viewer_position");
			objects.uniforms.push_back("light_position");
			add_shader(_shader_config, objects);

			ShaderDescriptionBlock animation(allocator, "animation");
			animation.stages.push_back("vert");
			animation.stages.push_back("frag");
			animation.attributes.push_back("in_position");
			animation.attributes.push_back("in_normal");
			animation.attributes.push_back("in_uv0");
			animation.attributes.push_back("in_blendindices");
			animation.attributes.push_back("in_blendweights");
			animation.uniforms.push_back("modelview_matrix");
			animation.uniforms.push_back("projection_matrix");
			animation.uniforms.push_back("object_matrix");
			animation.uniforms.push_back("node_transforms");
			animation.uniforms.push_back("inverse_bind_transforms");
			animation.uniforms.push_back("diffusemap");
			add_shader(_shader_config, animation);

			_shader_config->preprocessor_defines.push_back("#extension GL_ARB_explicit_attrib_location: require");
		} // startup

		void shutdown()
		{
			if (_shader_config)
			{
				MEMORY_DELETE(_shader_config, core::memory::global_allocator());
				_shader_config = nullptr;
			}
		} // shutdown

		bool fetch_shader_config(
			const char* shader_name,
			StringVector& attributes,
			StringVector& uniforms,
			StringVector& stages,
			StringVector& preprocessor)
		{
			if (!_shader_config->shaders.has_key(shader_name))
			{
				LOGV("Unable to find the shader block named \"%s\"\n", shader_name);
				return false;
			}

			ShaderDescriptionBlock& shader_block = _shader_config->shaders[shader_name];

			for (const core::StackString<32>& item : shader_block.attributes)
			{
				attributes.push_back(item());
			}

			for (const core::StackString<32>& item : shader_block.uniforms)
			{
				uniforms.push_back(item());
			}

			for (const core::StackString<32>& item : shader_block.stages)
			{
				stages.push_back(item());
			}

			// global preprocessor block
			for (const core::StackString<64>& item : _shader_config->preprocessor_defines)
			{
				preprocessor.push_back(item());
			}

			return true;
		}

		void map_attributes_and_uniforms(renderer::ShaderProgram* shader, StringVector& attributes, StringVector& uniforms)
		{
			if (shader->attributes.empty())
			{
				shader->attributes.allocate(attributes.size());
				size_t index = 0;
				for (const auto& value : attributes)
				{
					shader->attributes[index].first = value;
					shader->attributes[index].second = static_cast<int>(index);
					++index;
				}
			}

			if (shader->uniforms.empty())
			{
				shader->uniforms.allocate(uniforms.size());
				size_t index = 0;
				for (const auto& value : uniforms)
				{
					shader->uniforms[index].first = value;
					shader->uniforms[index].second = static_cast<int>(index);
					++index;
				}
			}
		} // map_attributes_and_uniforms


		bool verify_stages_exist_on_disk(const char* path, const StringVector& stages)
		{
			for (const auto& name : stages)
			{
				String filename = path;
				filename.append(".");
				filename.append(name);

				const render2::ResourceProvider* resource_provider = render2::get_resource_provider();
				if (!resource_provider->file_exists(filename.c_str()))
				{
					LOGE("\"%s\" does not exist!\n", filename.c_str());
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

		renderer::ShaderObject create_shader_from_file(gemini::Allocator& allocator, const char* shader_path, renderer::ShaderObjectType type, std::string& preprocessor_defines )
		{
			renderer::ShaderObject shader_object;

			Array<unsigned char> buffer(allocator);
			render2::ResourceProvider* resource_provider = render2::get_resource_provider();
			if (resource_provider->load_file(buffer, shader_path))
			{
				buffer.push_back('\0');
				StackString<32> version;
				render_utilities::strip_shader_version((char*)&buffer[0], version );
				if ( version._length == 0 )
				{
	#if defined(PLATFORM_GLES2_SUPPORT)
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

				if ( !renderer::driver()->shaderobject_compile( shader_object, (const char*)&buffer[0], preprocessor_defines.c_str(), version()) )
				{
					LOGE( "Error compiling shader!\n" );
				}
			}

			return shader_object;
		} // create_shader_from_file

		bool setup_program_with_config(gemini::Allocator& allocator, renderer::IRenderDriver* driver, renderer::ShaderProgram* program, const char* path, const StringVector& stages, std::string& preprocessor)
		{
			// verify all stages exist
			bool stages_exist = verify_stages_exist_on_disk(path, stages);
			if (!stages_exist)
			{
				LOGW("One or more stages do not exist for shader %s. Aborting.\n", path);
				return false;
			}

			// attach shader objects to program
			FixedArray<renderer::ShaderObject> shader_objects(allocator);
			shader_objects.allocate(stages.size());
			for (size_t i = 0; i < shader_objects.size(); ++i)
			{
				std::string filename = path + shader_stage_to_extension(stages[i]);
				shader_objects[i] = create_shader_from_file(allocator, filename.c_str(), shader_stage_to_shaderobject_type(stages[i]), preprocessor);
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

		void load_shaderprogram_from_file(gemini::Allocator& allocator, const char* path, renderer::ShaderProgram** shader_program)
		{
			// find the specific shader requested in the shader config
			// We could actually use the dirname here in case someone requests
			// a shader that doesn't reside in "shaders". Nah, unlikely.
			StackString<128> shader_path = path;
			StackString<128> shader_name = shader_path.basename();

			// look in the shader config json to find a shader that matches
			// load all attributes, uniforms, stages, and preprocessor defines we'll need.
			StringVector attributes, uniforms, stages, preprocessor;
			if (!fetch_shader_config(shader_name(), attributes, uniforms, stages, preprocessor))
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

			bool result = setup_program_with_config(allocator, driver, program, path, stages, preprocessor_defines);
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

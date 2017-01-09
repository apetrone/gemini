// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include <runtime/filesystem.h>
#include <runtime/runtime.h>
#include <core/stackstring.h>


#include "assets.h"
#include "renderer/renderer.h"
#include "assets/asset_shader.h"

using namespace renderer;

namespace gemini
{
	namespace assets
	{
		// -------------------------------------------------------------
		// Shader

		//void Shader::release()
		//{
		//	if (program)
		//	{
		//		driver()->shaderprogram_destroy( program );
		//	}
		//}



	#if 0
		AssetLoadStatus shader_load_callback(gemini::Allocator& allocator, const char* path, Shader* shader, const AssetParameters& parameters)
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

			shader->frag_data_location = "out_color";
			create_shader_attributes_and_uniforms(shader, attributes, uniforms);

			String preprocessor_defines;
			for (auto item : preprocessor)
			{
				preprocessor_defines += item;
				preprocessor_defines += "\n";
			}

			bool success = create_shader_program_from_file(path, stages, preprocessor_defines);
			if (!success)
			{
				return AssetLoad_Failure;
			}


	//		shader->show_attributes();

			return AssetLoad_Success;
		} // font_load_callback
	#endif

#if 0
		const platform::PathString& shader_prefix_path = gemini::assets::shaders()->get_prefix_path();

		// create shaders
		Array<unsigned char> vertex_shader_source(render_allocator);
		core::filesystem::instance()->virtual_load_file(vertex_shader_source, "shaders/150/vertexcolor.vert");
		assert(!vertex_shader_source.empty());

		render2::ShaderSource vertex_source;
		vertex_source.data = &vertex_shader_source[0];
		vertex_source.data_size = vertex_shader_source.size();
		vertex_source.stage_type = render2::SHADER_STAGE_VERTEX;

		Array<unsigned char> fragment_shader_source(render_allocator);
		core::filesystem::instance()->virtual_load_file(fragment_shader_source, "shaders/150/vertexcolor.frag");
		assert(!fragment_shader_source.empty());

		render2::ShaderSource frag_source;
		frag_source.data = &fragment_shader_source[0];
		frag_source.data_size = fragment_shader_source.size();
		frag_source.stage_type = render2::SHADER_STAGE_FRAGMENT;

		render2::ShaderSource* sources[] = { &vertex_source, &frag_source };
#endif

		AssetLoadStatus shader_load_callback(const char* path, AssetLoadState<render2::Shader>& load_state, const AssetParameters& parameters)
		{
			//render2::Shader* shader = load_state.asset;
			//create_shaderprogram_from_file(allocator, path, &shader->program);
			//if (!shader->program)
			{
				return AssetLoad_Failure;
			}

			return AssetLoad_Success;
		} // font_load_callback

		void shader_construct_extension(core::StackString<MAX_PATH_SIZE>& extension)
		{
		} // shader_construct_extension

		AssetLoadStatus shader_create_function(const char* path, AssetLoadState<render2::Shader>& load_state, const AssetParameters& parameters)
		{
			render2::Device* device = runtime_render_device();
			assert(device);

			LOGV("create shader \"%s\"\n", path);

			if (load_state.asset)
			{
				// re-load a shader
				assert(0); // not yet implemented!
			}
			else
			{
				// We could determine here how many stages we need to load.
				// I'll leave that as a TODO for now...

				platform::PathString asset_uri = path;
				asset_uri.append(".vert");

				// create shaders
				Array<unsigned char> vertex_shader_source(*load_state.allocator);
				core::filesystem::instance()->virtual_load_file(vertex_shader_source, asset_uri());
				assert(!vertex_shader_source.empty());

				render2::ShaderSource vertex_source;
				vertex_source.data = &vertex_shader_source[0];
				vertex_source.data_size = vertex_shader_source.size();
				vertex_source.stage_type = render2::SHADER_STAGE_VERTEX;

				asset_uri = path;
				asset_uri.append(".frag");
				Array<unsigned char> fragment_shader_source(*load_state.allocator);
				core::filesystem::instance()->virtual_load_file(fragment_shader_source, asset_uri());
				assert(!fragment_shader_source.empty());

				render2::ShaderSource frag_source;
				frag_source.data = &fragment_shader_source[0];
				frag_source.data_size = fragment_shader_source.size();
				frag_source.stage_type = render2::SHADER_STAGE_FRAGMENT;

				render2::ShaderSource* sources[] = { &vertex_source, &frag_source };

				load_state.asset = device->create_shader(sources, 2);
			}

			return AssetLoad_Success;
		} // shader_create_function

		void shader_destroy_function(AssetLoadState<render2::Shader>& load_state)
		{
			render2::Device* device = runtime_render_device();
			assert(device);

			device->destroy_shader(load_state.asset);
		} // shader_destroy_function
	} // namespace assets
} // namespace gemini

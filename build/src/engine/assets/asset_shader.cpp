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
#include <core/stackstring.h>
#include <core/filesystem.h>


#include "assets.h"
#include "renderer/renderer.h"
#include "assets/asset_shader.h"

namespace gemini
{
	namespace assets
	{
		// -------------------------------------------------------------
		// Shader
		
		void Shader::release()
		{
			if (program)
			{
				renderer::driver()->shaderprogram_destroy( program );
			}
		}

		

	#if 0
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


		AssetLoadStatus shader_load_callback(const char* path, Shader* shader, const AssetParameters& parameters)
		{
			renderer::create_shaderprogram_from_file(path, &shader->program);
			if (!shader->program)
			{
				return AssetLoad_Failure;
			}
			
			return AssetLoad_Success;
		} // font_load_callback

		void shader_construct_extension(StackString<MAX_PATH_SIZE>& extension)
		{
		} // shader_construct_extension
	} // namespace assets
} // namespace gemini
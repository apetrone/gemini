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
#include <core/mem.h>
#include <runtime/configloader.h>

#include <core/typedefs.h>

#include "assets.h"
#include "kernel.h"
#include <renderer/image.h>
#include <renderer/renderer.h>

#include <renderer/font_library.h>
#include <runtime/material_library.h>
#include <runtime/mesh_library.h>
#include <runtime/texture_library.h>
#include <renderer/shader_library.h>

using namespace renderer;

namespace gemini
{
	namespace assets
	{
		AssetParameters::~AssetParameters()
		{
		}

		unsigned int find_parameter_mask( ShaderString & name )
		{
			// TODO: need to validate the name here against the
			// parameter names in permutations config file.


	//		if ( _shader_permutations != 0 )
	//		{
	//			for( unsigned int option_id = 0; option_id < shader_permutations().num_permutations; ++option_id )
	//			{
	//				ShaderPermutationGroup * option = shader_permutations().options[ option_id ];
	//				if ( xstr_nicmp( (const char*)name.c_str(), option->name.c_str(), 64 ) == 0 )
	//				{
	//					//				LOGV( "mask for %s is %i\n", name.c_str(), option->mask_value );
	//					return (1 << option->mask_value);
	//				}
	//			}
	//		}

			LOGV("Unable to find parameter mask for %s\n", name.c_str());
			return 0;
		} // find_parameter_mask
	} // namespace assets


	struct AssetState
	{
		FontLibrary* fonts;
		MaterialLibrary* materials;
		MeshLibrary* meshes;
		gemini::ShaderLibrary* shaders;
		TextureLibrary* textures;
		render2::Device* device;
	};

	AssetState* _asset_state = nullptr;

	namespace assets
	{
		gemini::Allocator asset_allocator;

		// 1. Implement asset library
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(sounds)

		void load_default_texture_and_material()
		{
			image::Image default_image(asset_allocator);
			default_image.width = 128;
			default_image.height = 128;
			default_image.channels = 3;
			generate_checker_pattern(default_image, gemini::Color(1.0f, 0.0f, 1.0f), gemini::Color(0.0f, 1.0f, 0.0f));

			TextureCreateParameters params;
			params.device = _asset_state->device;
			params.filter = image::FILTER_NONE;
			render2::Texture* default_texture = _asset_state->textures->create(asset_allocator, &params);
			_asset_state->textures->default_asset(default_texture);
			_asset_state->textures->take_ownership("default", default_texture, false);

			// setup default material
			Material* default_material = _asset_state->materials->create(asset_allocator, nullptr);
			default_material->name = "default";

			MaterialParameter diffusemap;
			diffusemap.name = "diffusemap";
			diffusemap.type = MP_SAMPLER_2D;
			diffusemap.texture_unit = texture_unit_for_map(diffusemap.name);
			diffusemap.texture_handle = texture_load("default");

			default_material->add_parameter(diffusemap);

			_asset_state->materials->take_ownership("materials/default", default_material);
			_asset_state->materials->default_asset(default_material);
		} // load_default_texture_and_material


		void startup(render2::Device* device, bool load_default_assets)
		{
			assert(_asset_state == nullptr);

			assert(device != nullptr);

			platform::PathString shader_root = "shaders";
			shader_root.append(PATH_SEPARATOR_STRING);

			// TODO: Determine this by checking the renderer type
#if defined(PLATFORM_OPENGL_SUPPORT)
			shader_root.append("150");
#elif defined(PLATFORM_GLES2_SUPPORT)
			shader_root.append("100");
#endif
			// create the allocator
			asset_allocator = memory_allocator_default(MEMORY_ZONE_ASSETS);

			// allocate the base state
			_asset_state = MEMORY2_NEW(asset_allocator, AssetState)();
			_asset_state->device = device;

			// allocate each asset library
			_asset_state->fonts			= MEMORY2_NEW(asset_allocator, gemini::FontLibrary)(asset_allocator, device);
			_asset_state->materials		= MEMORY2_NEW(asset_allocator, gemini::MaterialLibrary)(asset_allocator, device);
			_asset_state->meshes		= MEMORY2_NEW(asset_allocator, gemini::MeshLibrary)(asset_allocator, device);
			_asset_state->shaders		= MEMORY2_NEW(asset_allocator, gemini::ShaderLibrary)(asset_allocator, device);
			_asset_state->textures		= MEMORY2_NEW(asset_allocator, gemini::TextureLibrary)(asset_allocator, device);

			_sounds						= MEMORY2_NEW(asset_allocator, soundsAssetLibrary)				(asset_allocator, sound_construct_extension, sound_load_callback);

			_asset_state->fonts->prefix_path("fonts");
			_asset_state->shaders->prefix_path(shader_root);

			if (load_default_assets)
			{
				load_default_texture_and_material();
			}
		} // startup

		void shutdown()
		{
			// Delete asset libraries
			MEMORY2_DELETE(asset_allocator, _sounds);

			MEMORY2_DELETE(asset_allocator, _asset_state->fonts);
			MEMORY2_DELETE(asset_allocator, _asset_state->materials);
			MEMORY2_DELETE(asset_allocator, _asset_state->meshes);
			MEMORY2_DELETE(asset_allocator, _asset_state->shaders);
			MEMORY2_DELETE(asset_allocator, _asset_state->textures);

			// Delete base asset state
			MEMORY2_DELETE(asset_allocator, _asset_state);
			_asset_state = nullptr;
		} // shutdown

		void append_asset_extension( AssetType type, core::StackString<MAX_PATH_SIZE> & path )
		{
			const char * extension = "";
#if defined(PLATFORM_IPHONEOS)
			kernel::KernelDeviceFlags device_flags = kernel::parameters().device_flags;
#endif

			switch( type )
			{
				case AssetType::Sound:
				{
#if defined(PLATFORM_IPHONEOS)
					if ( (device_flags & kernel::DeviceiPad) || (device_flags & kernel::DeviceiPhone) )
					{
						extension = "caf";
					}
#else
					extension = "ogg";
#endif
					break;
				} // Sound


				default: LOGW( "AssetType %i is NOT supported!\n" ); break;
			}

			path.append( "." );
			path.append( extension );

		} // append_asset_extension
	} // namespace assets

	AssetHandle mesh_load(const char* path, bool ignore_cache, void* parameters)
	{
		return _asset_state->meshes->load(path, ignore_cache, parameters);
	} // mesh_load

	Mesh* mesh_from_handle(AssetHandle handle)
	{
		return _asset_state->meshes->lookup(handle);
	} // mesh_from_handle

	AssetHandle shader_load(const char* path, bool ignore_cache, void* parameters)
	{
		return _asset_state->shaders->load(path, ignore_cache, parameters);
	} // shader_load

	render2::Shader* shader_from_handle(AssetHandle handle)
	{
		return _asset_state->shaders->lookup(handle);
	} // shader_from_handle

	AssetHandle texture_load(const char* path, bool ignore_cache, void* parameters)
	{
		return _asset_state->textures->load(path, ignore_cache, parameters);
	} // texture_load

	render2::Texture* texture_from_handle(AssetHandle handle)
	{
		return _asset_state->textures->lookup(handle);
	} // texture_from_handle

	AssetHandle material_load(const char* path, bool ignore_cache, void* parameters)
	{
		return _asset_state->materials->load(path, ignore_cache, parameters);
	} // material_load

	Material* material_from_handle(AssetHandle handle)
	{
		return _asset_state->materials->lookup(handle);
	} // material_from_handle

	size_t font_count_vertices(size_t string_length)
	{
		return string_length * 6;
	} // font_count_vertices

	size_t font_draw_string(AssetHandle handle, FontVertex* vertices, const char* utf8, size_t string_length, const Color& color)
	{
		return _asset_state->fonts->draw_string(handle, vertices, utf8, string_length, color);
	} // font_draw_string

	AssetHandle font_load(const char* path, bool ignore_cache, FontCreateParameters* parameters)
	{
		return _asset_state->fonts->load(path, ignore_cache, parameters);
	} // font_load

	void font_metrics(AssetHandle handle, FontMetrics& out_metrics)
	{
		return _asset_state->fonts->font_metrics(handle, out_metrics);
	} // font_metrics

	FontData* font_from_handle(AssetHandle handle)
	{
		return _asset_state->fonts->lookup(handle);
	} // font_from_handle

	render2::Texture* font_texture(AssetHandle handle)
	{
		return _asset_state->fonts->font_texture(handle);
	} // font_texture

	int32_t font_string_metrics(AssetHandle handle, const char* utf8, size_t string_length, glm::vec2& mins, glm::vec2& maxs)
	{
		return _asset_state->fonts->string_metrics(handle, utf8, string_length, mins, maxs);
	} // font_string_metrics

} // namespace gemini

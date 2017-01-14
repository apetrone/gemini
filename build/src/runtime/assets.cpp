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


	namespace assets
	{
		gemini::Allocator asset_allocator;

		// 1. Implement asset library
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(textures)
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(meshes)
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(materials)
		//IMPLEMENT_ASSET_LIBRARY_ACCESSOR(emitters)

		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(sounds)

		void load_default_texture_and_material()
		{
			assert(0); // TODO@apetrone (assets) fix this function
#if 0
			// setup default texture
			Texture* default_texture = textures()->allocate_asset();
			default_texture->texture = image::load_default_texture(default_texture->image);

			textures()->take_ownership("textures/default", default_texture);
			textures()->set_default(default_texture);
			LOGV( "Loaded default texture; asset_id = %i\n", default_texture->asset_id );

			// setup default material
			Material * default_material = materials()->allocate_asset();
			default_material->name = "default";

			MaterialParameter diffusemap;
			diffusemap.name = "diffusemap";
			diffusemap.type = MP_SAMPLER_2D;
			diffusemap.texture_unit = texture_unit_for_map(diffusemap.name);
			diffusemap.texture = default_texture->texture;

			default_material->add_parameter(diffusemap);

			materials()->take_ownership( "materials/default", default_material );
			materials()->set_default(default_material);
			LOGV( "Loaded default materials; asset_id = %i\n", default_material->asset_id );
#endif
		} // load_default_texture_and_material


		void startup()
		{
			asset_allocator = memory_allocator_default(MEMORY_ZONE_ASSETS);

			// 2. allocate asset libraries
			_textures =		MEMORY2_NEW(asset_allocator, texturesAssetLibrary)			(asset_allocator, texture_construct_extension, texture_load_callback);
			_meshes =		MEMORY2_NEW(asset_allocator, meshesAssetLibrary)				(asset_allocator, mesh_construct_extension, mesh_load_callback);
			_materials =	MEMORY2_NEW(asset_allocator, materialsAssetLibrary)			(asset_allocator, material_construct_extension, material_load_callback);
			//_emitters =		MEMORY2_NEW(asset_allocator, emitterConfigAssetLibrary)		(asset_allocator, emitterconfig_load_callback, emitterconfig_construct_extension);

			_sounds =		MEMORY2_NEW(asset_allocator, soundsAssetLibrary)				(asset_allocator, sound_construct_extension, sound_load_callback);

			//load_default_texture_and_material();
		} // startup

		void shutdown()
		{
			// 4. Delete asset library
			MEMORY2_DELETE(asset_allocator, _textures);
			MEMORY2_DELETE(asset_allocator, _meshes);
			MEMORY2_DELETE(asset_allocator, _materials);
			//MEMORY2_DELETE(asset_allocator, _emitters);
			//MEMORY2_DELETE(asset_allocator, _shaders);
			MEMORY2_DELETE(asset_allocator, _sounds);
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
} // namespace gemini

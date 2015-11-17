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
		void read_string_array( ShaderString ** array, unsigned int & num_items, Json::Value & root )
		{
			num_items = root.size();
			*array = MEMORY_NEW_ARRAY(ShaderString, num_items, core::memory::global_allocator());

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
		// 1. Implement asset library
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(TextureAssetLibrary, textures)
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(MeshAssetLibrary, meshes)
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(MaterialAssetLibrary, materials)
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(EmitterConfigAssetLibrary, emitters)
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(FontAssetLibrary, fonts)
		IMPLEMENT_ASSET_LIBRARY_ACCESSOR(ShaderAssetLibrary, shaders)

		void load_default_texture_and_material()
		{
			// setup default texture
			Texture * default_texture = textures()->allocate_asset();
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
			diffusemap.int_value = default_texture->Id();

			default_material->add_parameter(diffusemap);

			materials()->take_ownership( "materials/default", default_material );
			materials()->set_default(default_material);
			LOGV( "Loaded default materials; asset_id = %i\n", default_material->asset_id );

		} // load_default_texture_and_material


		void startup()
		{
			// 2. allocate asset libraries
			_textures = MEMORY_NEW(TextureAssetLibrary, core::memory::global_allocator()) (texture_load_callback, texture_construct_extension);
			_meshes = MEMORY_NEW(MeshAssetLibrary, core::memory::global_allocator()) (mesh_load_callback, mesh_construct_extension);
			_materials = MEMORY_NEW(MaterialAssetLibrary, core::memory::global_allocator()) (material_load_callback, material_construct_extension);
			_emitters = MEMORY_NEW(EmitterConfigAssetLibrary, core::memory::global_allocator()) (emitterconfig_load_callback, emitterconfig_construct_extension);
			_fonts = MEMORY_NEW(FontAssetLibrary, core::memory::global_allocator()) (font_load_callback, font_construct_extension);
			_shaders = MEMORY_NEW(ShaderAssetLibrary, core::memory::global_allocator()) (shader_load_callback, shader_construct_extension);

			load_default_texture_and_material();
		} // startup

		void shutdown()
		{
			// 4. Delete asset library
			MEMORY_DELETE(_textures, core::memory::global_allocator());
			MEMORY_DELETE(_meshes, core::memory::global_allocator());
			MEMORY_DELETE(_materials, core::memory::global_allocator());
			MEMORY_DELETE(_emitters, core::memory::global_allocator());
			MEMORY_DELETE(_fonts, core::memory::global_allocator());
			MEMORY_DELETE(_shaders, core::memory::global_allocator());
		} // shutdown

		void append_asset_extension( AssetType type, core::StackString<MAX_PATH_SIZE> & path )
		{
			const char * extension = "";
#if defined(PLATFORM_IPHONEOS)
			kernel::KernelDeviceFlags device_flags = kernel::parameters().device_flags;
#endif

			switch( type )
			{
				case SoundAsset:
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
				} // SoundAsset


				default: LOGW( "AssetType %i is NOT supported!\n" ); break;
			}

			path.append( "." );
			path.append( extension );

		} // append_asset_extension

	} // namespace assets
} // namespace gemini

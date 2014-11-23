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

#include <gemini/typedefs.h>
#include <gemini/mem.h>
#include <gemini/util/configloader.h>
#include <gemini/core/filesystem.h>

#include <slim/xlog.h>
#include <slim/xstr.h>
#include <slim/xtime.h>

#include "assets.h"
#include "kernel.h"
#include "image.h"
#include "renderer/renderer.h"

using namespace renderer;

namespace assets
{
	void read_string_array( ShaderString ** array, unsigned int & num_items, Json::Value & root )
	{
		num_items = root.size();
		*array = CREATE_ARRAY( ShaderString, num_items );
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
}; // namespace assets


namespace assets
{
	// 1. Implement asset library
	IMPLEMENT_ASSET_LIBRARY_ACCESSOR(TextureAssetLibrary, textures);
	IMPLEMENT_ASSET_LIBRARY_ACCESSOR(MeshAssetLibrary, meshes);
	IMPLEMENT_ASSET_LIBRARY_ACCESSOR(MaterialAssetLibrary, materials);
	IMPLEMENT_ASSET_LIBRARY_ACCESSOR(EmitterConfigAssetLibrary, emitters);
	IMPLEMENT_ASSET_LIBRARY_ACCESSOR(FontAssetLibrary, fonts);
	IMPLEMENT_ASSET_LIBRARY_ACCESSOR(ShaderAssetLibrary, shaders);
	
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

		Material::Parameter diffusemap;
		diffusemap.name = "diffusemap";
		diffusemap.type = MP_SAMPLER_2D;
		diffusemap.texture_unit = texture_unit_for_map(diffusemap.name);
		diffusemap.intValue = default_texture->Id();
		
		default_material->add_parameter(diffusemap);

		materials()->take_ownership( "materials/default", default_material );
		materials()->set_default(default_material);
		LOGV( "Loaded default materials; asset_id = %i\n", default_material->asset_id );
		
	} // load_default_texture_and_material


	void startup()
	{
		// 2. allocate asset libraries
		_textures = CREATE(TextureAssetLibrary, texture_load_callback, texture_construct_extension);
		_meshes = CREATE(MeshAssetLibrary, mesh_load_callback, mesh_construct_extension);
		_materials = CREATE(MaterialAssetLibrary, material_load_callback, material_construct_extension);
		_emitters = CREATE(EmitterConfigAssetLibrary, emitterconfig_load_callback, emitterconfig_construct_extension);
		_fonts = CREATE(FontAssetLibrary, font_load_callback, font_construct_extension);
		_shaders = CREATE(ShaderAssetLibrary, shader_load_callback, shader_construct_extension);
		
		// load shader config
		create_shader_config();

		load_default_texture_and_material();
	} // startup
	
	void shutdown()
	{
		destroy_shader_config();
		
		// 4. Delete asset library
		DESTROY(TextureAssetLibrary, _textures);
		DESTROY(MeshAssetLibrary, _meshes);
		DESTROY(MaterialAssetLibrary, _materials);
		DESTROY(EmitterConfigAssetLibrary, _emitters);
		DESTROY(FontAssetLibrary, _fonts);
		DESTROY(ShaderAssetLibrary, _shaders);
	} // shutdown

	void append_asset_extension( AssetType type, StackString<MAX_PATH_SIZE> & path )
	{
		const char * extension = "";
		kernel::KernelDeviceFlags device_flags = kernel::instance()->parameters().device_flags;
		
		switch( type )
		{
			case SoundAsset:
			{
#if PLATFORM_APPLE && PLATFORM_IOS
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
		
}; // namespace assets
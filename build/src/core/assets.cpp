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
#include "typedefs.h"
#include "assets.hpp"
#include "assetlibrary.hpp"
#include "kernel.hpp"
#include "image.hpp"
#include "log.h"

// these may have to be moved
namespace assets
{
	// -------------------------------------------------------------
	// Textures
	void Texture::release()
	{
		image::driver_release_image( this->texture_id );
	} // release
}; // namespace assets





namespace assets
{
	Texture * _default_texture = 0;

	AssetLoadStatus texture_load_callback( const char * path, Texture * texture, unsigned int flags )
	{
		unsigned int texture_id;
		unsigned int width;
		unsigned int height;
		
		bool load_result = 0;
		
		if ( !(flags & image::F_CUBEMAP) ) // load 2d texture
		{
			load_result = image::load_image_from_file( path, texture_id, flags, &width, &height );
		}
		else // load cubemap
		{
			StackString< MAX_PATH_SIZE > fullpath[6];
			const char ext[][4] = { "_rt", "_lt", "_up", "_dn", "_ft", "_bk" };
			const char * names[6];
			for( int i = 0; i < 6; ++i )
			{
				fullpath[i] = path;
				fullpath[i].remove_extension();
				fullpath[i].append( ext[i] );
				assets::append_asset_extension( TextureAsset, fullpath[i] );
				names[i] = fullpath[i]();
			}
			
//			load_result = renderlib::LoadCubemap( names, texture_id, flags, &width, &height );
			assert( 0 );
		}
		
		if ( load_result )
		{
			texture->flags = flags;
			texture->texture_id = texture_id;
			texture->width = width;
			texture->height = width;
			return assets::AssetLoad_Success;
		}

		return assets::AssetLoad_Failure;
	}
	
	typedef AssetLibrary< Texture, TextureAsset> TextureAssetLibrary;
	
	TextureAssetLibrary * texture_lib;

	void startup()
	{
		texture_lib = CREATE(TextureAssetLibrary, texture_load_callback);

		_default_texture = texture_lib->allocate_asset();
		_default_texture->texture_id = image::load_default_texture();
		texture_lib->take_ownership("textures/default", _default_texture);
		LOGV( "Loaded default texture; id = %i\n", _default_texture->texture_id );
	} // startup
	
	void purge()
	{
		texture_lib->release_and_purge();
	} // purge
	
	void shutdown()
	{
		purge();
		DESTROY(TextureAssetLibrary, texture_lib);
	} // shutdown

	void append_asset_extension( AssetType type, StackString<MAX_PATH_SIZE> & path )
	{
		const char * extension = "";
		kernel::KernelDeviceFlags device_flags = kernel::instance()->parameters().device_flags;
		
		switch( type )
		{
			case SoundAsset:
			{
#if PLATFORM_IS_MOBILE
				if ( (device_flags & kernel::DeviceiPad) || (device_flags & kernel::DeviceiPhone) )
				{
					extension = "caf";
				}
#else
				;
				extension = "ogg";
#endif
				break;
			} // SoundAsset
			
			case TextureAsset:
			{
				if ( device_flags & kernel::DeviceDesktop )
				{
					// ...
					extension = "png";
				}
				else if ( device_flags & kernel::DeviceiPhone )
				{
					extension = "png";
					path.append( "-iphone" );
				}
				else if ( device_flags & kernel::DeviceiPad )
				{
					extension = "png";
					path.append( "-ipad" );
				}
				
				if ( device_flags & kernel::DeviceSupportsRetinaDisplay )
				{
					path.append( "@2x" );
				}
				
				break;
			} // TextureAsset
			default: LOGW( "AssetType %i is NOT supported!\n" ); break;
		}
		
		path.append( "." );
		path.append( extension );
		
	} // append_asset_extension
	
	
	Texture * load_texture( const char * path, unsigned int flags, bool ignore_cache )
	{
		assert( texture_lib != 0 );
		Texture * texture = texture_lib->load_from_path( path, flags, ignore_cache );
		if ( texture )
		{
			return texture;
		}
		
		return _default_texture;
	} // load_texture
	
}; // namespace assets
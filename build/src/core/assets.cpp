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

#include "assets.hpp"
#include "assetlibrary.hpp"
#include "kernel.hpp"


namespace assets
{

	AssetLoadStatus texture_load_callback( const char * path, Texture * texture, unsigned int flags )
	{
		return assets::Failure;
	}
	
	AssetLibrary< Texture, TextureAsset> texture_lib( texture_load_callback );
	








	void construct_absolute_path_from_relative_path( AssetType type, StackString<MAX_PATH_SIZE> & path )
	{
		const char * extension = "";
		switch( type )
		{
			case SoundAsset:
			{
#if PLATFORM_IS_MOBILE
				if ( (kernel::instance()->parameters().device_flags & kernel::DeviceiPad) || (kernel::instance()->parameters().device_flags & kernel::DeviceiPhone) )
				{
					extension = "caf";
				}
#else
				;
				extension = "ogg";
#endif
				break;
			} // SoundAsset
			
			default: break;
		}
		
		path.append( "." );
		path.append( extension );
		
	} // absolute_path_from
	
	
	Texture * load_texture( const char * path, unsigned int flags, bool ignore_cache )
	{
		Texture * texture = texture_lib.load_from_path( path, flags, ignore_cache );
		if ( texture )
		{
			return texture;
		}
		
		return 0;
	} // load_texture
	
}; // namespace assets
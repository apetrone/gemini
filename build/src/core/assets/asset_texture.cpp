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

#include "platform.hpp"
#include "stackstring.hpp"

#include "mathlib.h" // for glm
#include "color.hpp"
#include "renderer.hpp"

#include <string>
#include "assets.hpp"
#include "assets/asset_texture.hpp"

#include "kernel.hpp" // for device_flags

namespace assets
{
	void Texture::release()
	{
		image::driver_release_image( this->texture_id );
	} // release
	
	AssetLoadStatus texture_load_callback( const char * path, Texture * texture, unsigned int flags )
	{
		unsigned int texture_id = 0;
		unsigned int width = 0;
		unsigned int height = 0;
		
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
				assets::textures()->append_extension( fullpath[i] );
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
	} // texture_load_callback
	
	
	void texture_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		kernel::KernelDeviceFlags device_flags = kernel::instance()->parameters().device_flags;
		const char * ext = 0;
		
		if ( device_flags & kernel::DeviceDesktop || (device_flags & kernel::DeviceAndroid) )
		{
			// ...
			ext = "png";
		}
		else if ( device_flags & kernel::DeviceiPhone )
		{
			ext = "png";
			extension.append( "-iphone" );
		}
		else if ( device_flags & kernel::DeviceiPad )
		{
			ext = "png";
			extension.append( "-ipad" );
		}
		
		if ( device_flags & kernel::DeviceSupportsRetinaDisplay )
		{
			extension.append( "@2x" );
		}

		extension.append(".");
		extension.append(ext);
	} // texture_construct_extension

}; // namespace assets

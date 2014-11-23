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
#include <string>

#include <gemini/util/stackstring.h>
#include <gemini/platform.h>
#include <gemini/core/filesystem.h>

#include <gemini/mathlib.h> // for glm
#include "color.h"
#include "renderer/renderer.h"
#include "assets.h"
#include "assets/asset_texture.h"
#include "image.h"

#include "kernel.h" // for device_flags

namespace assets
{
	Texture::Texture() : texture(0) {}
	Texture::~Texture() {}
	
	void Texture::release()
	{
		renderer::driver()->texture_destroy(this->texture);
	} // release

	
	AssetLoadStatus texture_load_callback( const char * path, Texture * texture, const TextureParameters & parameters )
	{
		unsigned int texture_id = 0;
		unsigned int width = 0;
		unsigned int height = 0;
		bool load_result = 0;
		unsigned int flags = 0;
		
		if ( !(parameters.flags & image::F_CUBEMAP) ) // load 2d texture
		{
			texture->texture = load_texture_from_file(path, parameters.flags, texture->image);
			load_result = texture->texture != 0;
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
			texture->image.flags = parameters.flags;
			texture->image.width = width;
			texture->image.height = width;
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

	renderer::Texture* load_texture_from_file(const char * filename, unsigned int flags, image::Image& image)
	{
		size_t buffer_size = 0;
		char * filedata;
		
		renderer::Texture* render_texture = nullptr;
		
		filedata = core::filesystem::file_to_buffer( filename, 0, &buffer_size );
		
		if ( filedata )
		{
			unsigned char * pixels = image::load_image_from_memory((unsigned char*)filedata, buffer_size, &image.width, &image.height, &image.channels );
			if ( pixels )
			{
//				may need to actually flip the image vertically here
//				flip_image_vertically( width, height, components, pixels );

				image.pixels = pixels;
				render_texture = renderer::driver()->texture_create(image);
				
				LOGV( "Loaded texture \"%s\"; (%i x %i @ %ibpp)\n", filename, image.width, image.height, image.channels );
				
				image::free_image( pixels );
			}
			else
			{
				LOGE( "Unable to load image %s\n", filename );
			}
			
			DEALLOC(filedata);
			return render_texture;
		}
		else
		{
			LOGE( "Couldn't load file: %s\n", filename );
			return nullptr;
		}
		
		return nullptr;
	} // load_texture_from_file

}; // namespace assets

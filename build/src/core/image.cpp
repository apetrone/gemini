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
#include <gemini/core/filesystem.h>

#include <slim/xlog.h>

#include "image.h"
#include "renderer.h"

#define STBI_HEADER_FILE_ONLY 1
#define STBI_NO_STDIO 1
#include "stb_image.c"

namespace image
{
	// given two colors, generate an alternating checker pattern image
	void generate_checker_image( unsigned char * pixels, int width, int height, const Color & color1, const Color & color2 )
	{
		// width/height should be power of two
		int width_mask = (width >> 1) - 1;
		int height_mask = (height >> 1) - 1;
		
		const Color * color = 0;
	
		assert( pixels != 0 );

		for( int y = 0; y < height; ++y )
		{
			for( int x = 0; x < width; ++x )
			{
				// less than half
				if ( y <= height_mask )
				{
					// less than half
					if ( x <= width_mask )
					{
						color = &color1;
					}
					else
					{
						color = &color2;
					}
				}
				else
				{
					// less than half
					if ( x <= width_mask )
					{
						color = &color2;
					}
					else
					{
						color = &color1;
					}
				}
				
				// we want this to be an opaque texture, so we'll ignore the alpha channel
				memcpy( pixels, color, 3 );
				
				pixels += 3;
			}
		}
	} // generate_texture_image

	// flip an image vertically - this uses heap space to create a copy, but deletes it when finished
	void flip_image_vertically( int width, int height, int components, unsigned char * pixels )
	{
		int scanline_size = width*components;
		int dst = 0;
		unsigned char * copy;
		copy = (unsigned char*)ALLOC( (width*height*components) );
		memcpy( copy, pixels, (width*height*components) );
		
		for( int h = 0; h < height; ++h )
		{
			dst = (height-(h+1));
			memcpy( &pixels[ (h*scanline_size) ], &copy[ (dst*scanline_size) ], scanline_size );
		}
		
		DEALLOC(copy);
	} // flip_image_vertically


	unsigned int load_default_texture()
	{
		unsigned char pixels[ ERROR_TEXTURE_WIDTH * ERROR_TEXTURE_HEIGHT * 3 ];
		unsigned int texture_id = 0;
	
		generate_checker_image( pixels, ERROR_TEXTURE_HEIGHT, ERROR_TEXTURE_WIDTH, Color(128, 128, 128), Color(96, 96, 96) );
	
		driver_upload_image2d( texture_id, 0, ERROR_TEXTURE_WIDTH, ERROR_TEXTURE_HEIGHT, 3, pixels );
		return texture_id;
	} // load_default_texture
	
	
	bool load_image_from_file( const char * filename, unsigned int & texID, unsigned int flags, unsigned int * out_width, unsigned int * out_height )
	{
		size_t buffer_size = 0;
		char * filedata;
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int components = 0;

		filedata = core::filesystem::file_to_buffer( filename, 0, &buffer_size );
		
		if ( filedata )
		{
			unsigned char * pixels = load_image_from_memory((unsigned char*)filedata, buffer_size, &width, &height, &components );
			if ( pixels )
			{
				if ( out_width )
				{
					*out_width = width;
				}
				
				if ( out_height )
				{
					*out_height = height;
				}
				
				// may need to actually flip the image vertically here
//				flip_image_vertically( width, height, components, pixels );
				
				// upload texture to video card
				driver_upload_image2d( texID, flags, width, height, components, pixels );
				
//				LOGV( "Loaded texture \"%s\"; Texture ID: %i, (%i x %i @ %ibpp)\n", filename, texID, width, height, components );
				
				free_image( pixels );
			}
			else
			{
				LOGE( "Unable to load image %s\n", filename );
			}
			
			DEALLOC(filedata);
			return true;
		}
		else
		{
			LOGE( "Couldn't load file: %s\n", filename );
			return false;
		}
		
		return false;
	} // load_image_from_file
	
	
	
	unsigned char * load_image_from_memory( unsigned char * data, unsigned int data_size, unsigned int * width, unsigned int * height, unsigned int * channels )
	{
		unsigned char * pixels = 0;
		int w, h, c;

		pixels = stbi_load_from_memory( data, data_size, &w, &h, &c, 0 );
		*width = w;
		*height = h;
		*channels = c;	
		
		return pixels;
	} // load_image_from_memory
	
	void free_image( unsigned char * pixels )
	{
		// this was not allocated by our allocator (was done through stb_image)
		// so must not ask our deallocator to delete it.
		stbi_image_free( pixels );
	} // free_image



	// driver interaction
	
	
	void driver_release_image( unsigned int texture_id )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		if ( !driver )
		{
			LOGE( "upload_texture_2d called with no active render driver!\n" );
			return;
		}
		
		// populate texture parameter struct
		renderer::TextureParameters params;
		params.texture_id = texture_id;
		
		driver->destroy_texture( params );
	} // driver_release_image


	void driver_upload_image2d( unsigned int & texture_id, unsigned int flags, unsigned int width, unsigned int height, unsigned int channels, unsigned char * pixels )
	{
		renderer::IRenderDriver * driver = renderer::driver();
		if ( !driver )
		{
			LOGE( "upload_texture_2d called with no active render driver!\n" );
			return;
		}
		
		// populate texture parameter struct
		renderer::TextureParameters params;
		params.texture_id = texture_id;
		params.image_flags = flags;
		params.width = width;
		params.height = height;
		params.channels = channels;
		params.pixels = pixels;
		params.alignment = 4;
		
		// we want to support re-using an existing image if possible; so let's see if the renderer is already aware this is a texture.
		if ( !driver->is_texture(params) )
		{
			// not already a texture; so generate one
			assert( driver->generate_texture(params) );
		}
		
		// generate 2d texture with render driver
		assert( driver->upload_texture_2d(params) );

		// retrieve texture id from driver
		texture_id = params.texture_id;
	} // driver_upload_image2d

}; // namespace image
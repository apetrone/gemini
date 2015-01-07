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
#include <core/typedefs.h>
#include <core/filesystem.h>

#include <slim/xlog.h>

#include "image.h"
#include "renderer.h"

#define STBI_HEADER_FILE_ONLY 1
#define STBI_NO_STDIO 1
#include "stb_image.c"

namespace image
{
	Image::Image()
	{
		type = IT_UNKNOWN;
		filter = FILTER_NONE;
		flags = 0;
		
		width = 0;
		height = 0;
		
		channels = 3;
	}

	void generate_checker_pattern(Image& image, const Color & color1, const Color & color2)
	{
		// image dimensions must be specified
		assert((image.width > 0) && (image.height > 0));
		
		// only generate 3 channel RGB image for now.
		assert(image.channels == 3);
		
		
		image.pixels.allocate(image.channels*image.width*image.height);
		
		// width/height should be power of two
		int width_mask = (image.width >> 1) - 1;
		int height_mask = (image.height >> 1) - 1;
		
		const Color * color = 0;
		uint8_t* pixels = &image.pixels[0];
		assert( pixels != 0 );
		
		for(uint32_t y = 0; y < image.height; ++y)
		{
			for(uint32_t x = 0; x < image.width; ++x)
			{
				// less than half
				if (y <= height_mask)
				{
					// less than half
					if (x <= width_mask)
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
					if (x <= width_mask)
					{
						color = &color2;
					}
					else
					{
						color = &color1;
					}
				}
				
				memcpy(pixels, color, image.channels);
				
				pixels += image.channels;
			}
		}
	}

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


	renderer::Texture* load_default_texture(Image& image)
	{
		image.width = ERROR_TEXTURE_WIDTH;
		image.height = ERROR_TEXTURE_HEIGHT;
		image.channels = 3;
		generate_checker_pattern(image, Color(0, 0, 0), Color(255, 0, 255));
		
		renderer::Texture* texture = renderer::driver()->texture_create(image);

		return texture;

	} // load_default_texture
	
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

}; // namespace image
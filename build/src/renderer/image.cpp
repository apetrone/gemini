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
#include "image.h"
#include "renderer.h"

#include <runtime/filesystem.h>
#include <runtime/logging.h>

#include <core/typedefs.h>

#define STBI_HEADER_FILE_ONLY 1
#define STBI_NO_STDIO 1
#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"

using namespace core;

namespace image
{
	Image::Image()
	{
		type = image::TEX_2D;
		filter = FILTER_NONE;
		flags = 0;
		
		width = 0;
		height = 0;
		
		channels = 3;
		alignment = 4;
	}
	
	void Image::create(const uint32_t& image_width, const uint32_t& image_height, const uint32_t& total_channels)
	{
		width = image_width;
		height = image_height;
		channels = total_channels;
		
		pixels.allocate(width * height * channels);
		alignment = channels;
	}
	
	void Image::fill(const core::Color& color)
	{
		uint8_t* pixel = &pixels[0];
		for (size_t h = 0; h < height; ++h)
		{
			for (size_t w = 0; w < width; ++w)
			{
				memcpy(pixel, &color, channels);
				pixel += channels;
			}
		}
	}

	void Image::copy(const uint8_t* buffer, const uint32_t& width, const uint32_t& height, const uint32_t& pitch, uint32_t border)
	{
		// It is assumed, that if border > 0; then this Image's
		// dimensions are already set to include that border.

		// It is also assumed that the source buffer and destination buffer match
		// channels.

		// this must already be allocated -- allocate here?
		assert(!this->pixels.empty());

		unsigned char* img;

		// if you hit either of these asserts, the source buffer is larger than
		// this image's buffer. We cannot copy into this without losing data.
		assert((width + (2*border)) <= this->width);
		assert((height + (2*border)) <= this->height);

		size_t local_pitch = (this->width * channels);
		for (size_t h = 0; h < height; ++h)
		{
			for (size_t w = 0; w < width; ++w)
			{
				img = &pixels[((h + border) * local_pitch + ((w + border) * channels))];
				unsigned char* x = (unsigned char*)&buffer[ ((height-1 - h) * pitch) + (w) ];
				memcpy(img, x, channels);
			}
		}
	}

	void generate_checker_pattern(Image& image, const Color & color1, const Color & color2)
	{
		// image dimensions must be specified
		assert((image.width > 0) && (image.height > 0));
		
		// only generate 3 channel RGB image for now.
		assert(image.channels == 3);
		
		
		image.pixels.allocate(image.channels*image.width*image.height);
		
		// width/height should be power of two
		uint32_t width_mask = (image.width >> 1) - 1;
		uint32_t height_mask = (image.height >> 1) - 1;
		
		const Color * color = 0;
		uint8_t* pixels = &image.pixels[0];
		assert(pixels != 0);
		
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
	} // generate_checker_pattern

	// given two colors, generate an alternating checker pattern image
	void generate_checker_image(unsigned char* pixels, int width, int height, const Color& color1, const Color& color2)
	{
		// width/height should be power of two
		int width_mask = (width >> 1) - 1;
		int height_mask = (height >> 1) - 1;
		
		const Color* color = 0;
	
		assert(pixels != 0);

		for(int y = 0; y < height; ++y)
		{
			for(int x = 0; x < width; ++x)
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
				
				// we want this to be an opaque texture, so we'll ignore the alpha channel
				memcpy(pixels, color, 3);
				
				pixels += 3;
			}
		}
	} // generate_texture_image

	// flip an image vertically - this uses heap space to create a copy, but deletes it when finished
	void flip_image_vertically(int width, int height, int components, unsigned char* pixels)
	{
		int scanline_size = width*components;
		int dst = 0;
		unsigned char* copy;
		copy = (unsigned char*)MEMORY_ALLOC((width*height*components), core::memory::global_allocator());
		memcpy(copy, pixels, (width*height*components));
		
		for(int h = 0; h < height; ++h)
		{
			dst = (height-(h+1));
			memcpy(&pixels[ (h*scanline_size) ], &copy[ (dst*scanline_size) ], scanline_size);
		}
		
		MEMORY_DEALLOC(copy, core::memory::global_allocator());
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

	Image load_from_memory(unsigned char* data, unsigned int data_size)
	{
		Image image;
		int width;
		int height;
		int channels;

		unsigned char* pixels = stbi_load_from_memory(data, data_size, &width, &height, &channels, 0);

		image.width = width;
		image.height = height;
		image.channels = channels;

		// pixels returned by stbi are uncompressed; so do a raw alloc and copy.
		size_t pixels_size = image.width * image.height * image.channels;
		image.pixels.allocate(pixels_size);
		memcpy(&image.pixels[0], pixels, pixels_size);

		return image;
	}

	unsigned char* load_image_from_memory(unsigned char* data, unsigned int data_size, unsigned int* width, unsigned int* height, unsigned int* channels)
	{
		unsigned char* pixels = 0;
		int w, h, c;

		pixels = stbi_load_from_memory(data, data_size, &w, &h, &c, 0);
		*width = w;
		*height = h;
		*channels = c;
		
		return pixels;
	} // load_image_from_memory
	
	void free_image(unsigned char* pixels)
	{
		// this was not allocated by our allocator (was done through stb_image)
		// so must not ask our deallocator to delete it.
		stbi_image_free(pixels);
	} // free_image

} // namespace image
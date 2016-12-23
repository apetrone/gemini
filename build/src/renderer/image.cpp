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

#include <core/logging.h>
#include <core/typedefs.h>

#define STBI_HEADER_FILE_ONLY 1
#define STBI_NO_STDIO 1
#define STB_IMAGE_IMPLEMENTATION 1

#if defined(PLATFORM_COMPILER_MSVC)
	// disable some warnings in stb_image
	#pragma warning(push)
	#pragma warning(disable: 4312) // 'type cast': conversion from 'int' to 'unsigned char *' of greater size
	#pragma warning(disable: 4365) // 'argument': conversion from 'int' to 'size_t', signed/unsigned mismatch
	#pragma warning(disable: 4456) // declaration of 'k' hides previous local declaration
	#pragma warning(disable: 4457) // declaration of 'y' hides function parameter
#endif

#include "stb_image.h"

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(pop)
#endif



using namespace core;

namespace image
{
	const unsigned int ERROR_TEXTURE_WIDTH = 128;
	const unsigned int ERROR_TEXTURE_HEIGHT = 128;

	Image::Image(gemini::Allocator& allocator)
		: pixels(allocator)
	{
		type = image::TEX_2D;
		filter = FILTER_NONE;
		flags = 0;

		width = 0;
		height = 0;

		channels = 3;
		alignment = 4;
	}

	Image::Image(const Image& other)
		: pixels(other.pixels)
	{
		type = other.type;
		filter = other.filter;
		flags = other.flags;
		width = other.width;
		height = other.height;
		channels = other.channels;
		alignment = other.alignment;
	}

	Image::~Image()
	{
		// shutting down image.
		int z = 302;
		pixels.clear();
	}

	Image& Image::operator=(const Image& other)
	{
		type = other.type;
		filter = other.filter;
		flags = other.flags;
		width = other.width;
		height = other.height;
		channels = other.channels;
		alignment = other.alignment;

		// invoke assignment operator on array
		pixels = other.pixels;

		return *this;
	}

	void Image::create(const uint32_t& image_width, const uint32_t& image_height, const uint32_t& total_channels)
	{
		width = image_width;
		height = image_height;
		channels = total_channels;

		pixels.allocate(width * height * channels);
		alignment = channels;
	}

	void Image::fill(const gemini::Color& color)
	{
		uint8_t* pixel = &pixels[0];
		const uint8_t red = static_cast<uint8_t>(static_cast<int>(color.red * 255.0f) & 0xff);
		const uint8_t green = static_cast<uint8_t>(static_cast<int>(color.green * 255.0f) & 0xff);
		const uint8_t blue = static_cast<uint8_t>(static_cast<int>(color.blue * 255.0f) & 0xff);
		const uint8_t alpha = static_cast<uint8_t>(static_cast<int>(color.alpha * 255.0f) & 0xff);

		// We don't handle cases outside this.
		assert(channels <= 4);

		for (size_t h = 0; h < height; ++h)
		{
			for (size_t w = 0; w < width; ++w)
			{
				pixel[0] = red;
				if (channels > 1)
				{
					pixel[1] = green;
					pixel[2] = blue;

					if (channels == 4)
					{
						pixel[3] = alpha;
					}
				}
				pixel += channels;
			}
		}
	}

	void Image::copy(const uint8_t* buffer, const uint32_t& fill_width, const uint32_t& fill_height, const uint32_t& pitch, uint32_t border)
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
		assert((fill_width + (2*border)) <= width);
		assert((fill_height + (2*border)) <= height);

		size_t local_pitch = (width * channels);
		for (size_t h = 0; h < fill_height; ++h)
		{
			for (size_t w = 0; w < fill_width; ++w)
			{
				img = &pixels[((h + border) * local_pitch + ((w + border) * channels))];
				unsigned char* x = (unsigned char*)&buffer[ ((fill_height -1 - h) * pitch) + (w) ];
				memcpy(img, x, channels);
			}
		}
	}

	void generate_checker_pattern(Image& image, const gemini::Color & color1, const gemini::Color & color2)
	{
		// image dimensions must be specified
		assert((image.width > 0) && (image.height > 0));

		// only generate 3 channel RGB image for now.
		assert(image.channels == 3);

		image.pixels.allocate(image.channels*image.width*image.height);

		// width/height should be power of two
		uint32_t width_mask = (image.width >> 1) - 1;
		uint32_t height_mask = (image.height >> 1) - 1;

		const gemini::Color * color = 0;
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

				pixels[0] = static_cast<uint8_t>(static_cast<int>(color->red * 255.0f) & 0xff);
				pixels[1] = static_cast<uint8_t>(static_cast<int>(color->green * 255.0f) & 0xff);
				pixels[2] = static_cast<uint8_t>(static_cast<int>(color->blue * 255.0f) & 0xff);
				pixels += image.channels;
			}
		}
	} // generate_checker_pattern

	// given two colors, generate an alternating checker pattern image
	void generate_checker_image(unsigned char* pixels, int width, int height, const gemini::Color& color1, const gemini::Color& color2)
	{
		// width/height should be power of two
		int width_mask = (width >> 1) - 1;
		int height_mask = (height >> 1) - 1;

		const gemini::Color* color = 0;

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
		size_t scanline_size = static_cast<size_t>(width * components);
		const size_t image_size_bytes = static_cast<size_t>(width * height * components);
		int dst = 0;
		gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
		unsigned char* copy = static_cast<unsigned char*>(MEMORY2_ALLOC(allocator, image_size_bytes));
		memcpy(copy, pixels, image_size_bytes);

		for(int h = 0; h < height; ++h)
		{
			dst = (height-(h+1));
			memcpy(&pixels[ (h*scanline_size) ], &copy[ (dst*scanline_size) ], scanline_size);
		}

		MEMORY2_DEALLOC(allocator, copy);
	} // flip_image_vertically


	renderer::Texture* load_default_texture(Image& image)
	{
		image.width = ERROR_TEXTURE_WIDTH;
		image.height = ERROR_TEXTURE_HEIGHT;
		image.channels = 3;
		generate_checker_pattern(image, gemini::Color(1.0f, 0, 1.0f), gemini::Color(0, 1.0f, 0));

		renderer::Texture* texture = renderer::driver()->texture_create(image);

		return texture;
	} // load_default_texture

	Image load_from_memory(gemini::Allocator& allocator, unsigned char* data, unsigned int data_size)
	{
		Image image(allocator);
		int width;
		int height;
		int channels;

		unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(data_size), &width, &height, &channels, 0);

		image.width = static_cast<uint32_t>(width);
		image.height = static_cast<uint32_t>(height);
		image.channels = static_cast<uint32_t>(channels);

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

		pixels = stbi_load_from_memory(data, static_cast<int>(data_size), &w, &h, &c, 0);
		*width = static_cast<unsigned int>(w);
		*height = static_cast<unsigned int>(h);
		*channels = static_cast<unsigned int>(c);

		return pixels;
	} // load_image_from_memory

	void free_image(unsigned char* pixels)
	{
		// this was not allocated by our allocator (was done through stb_image)
		// so must not ask our deallocator to delete it.
		stbi_image_free(pixels);
	} // free_image

} // namespace image

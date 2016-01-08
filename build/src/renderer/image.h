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
#pragma once

#include "color.h"

#include <core/typedefs.h>
#include <core/fixedarray.h>

namespace renderer
{
	struct Texture;
}

namespace image
{
	// image format types - which currently map directly to SOIL formats.
	enum ImageFormatType
	{
		IT_TGA = 0,
		IT_BMP = 1,
		IT_DDS = 2
	}; // ImageFormatType

	enum ImageType
	{
		TEX_2D = 0,
		TEX_CUBE = 1
	}; // ImageType

	enum ImageFlags
	{
		F_RGB = 1,
		F_RGBA = 2,
		F_ALPHA = 4,
		F_WRAP = 8,				// wrap out of bounds UV coords
		F_CLAMP = 16,			// clamp out of bounds UV coords,
		F_CLAMP_BORDER = 32,	// requires GL_ARB_texture_border_clamp
		F_CUBEMAP = 64,			// load this texture as a cubemap,
		F_NO_MIPMAPS = 128,		// image should not have mipmaps
	}; // ImageFlags

	enum FilterType
	{
		FILTER_NONE,				// nearest
		FILTER_LINEAR,				// linear
		FILTER_LINEAR_MIPMAP,		// linear, with mipmapping
	};


	// ---------------------------------------------------------------------
	// Image
	// ---------------------------------------------------------------------
#if 0
	struct Image
	{
		// color components
		// ordering
		// component size
		// presence or absence of compression
		uint32_t mip_level;
		uint32_t pixel_format;

		void update_pixels(const Region& rect, size_t miplevel, void* bytes, size_t bytes_per_row);
	};
#endif

	struct Image
	{
		ImageType type;
		FilterType filter;
		uint32_t flags;

		uint32_t width;
		uint32_t height;

		// bytes per pixel
		uint32_t channels;

		// rows are aligned to this value
		uint32_t alignment;

		// can be raw pixel data, or compressed data, etc.
		FixedArray<uint8_t> pixels;

		LIBRARY_EXPORT Image();
		LIBRARY_EXPORT void create(const uint32_t& image_width, const uint32_t& image_height, const uint32_t& total_channels);
		LIBRARY_EXPORT void fill(const core::Color& color);

		// copy an image from a target buffer
		LIBRARY_EXPORT void copy(const uint8_t* pixels, const uint32_t& width, const uint32_t& height, const uint32_t& pitch, uint32_t border = 0);
	};

	LIBRARY_EXPORT void generate_checker_pattern(Image& image, const core::Color& color1, const core::Color& color2);
	LIBRARY_EXPORT void generate_checker_image(unsigned char* pixels, int width, int height, const core::Color& color1, const core::Color& color2);
	LIBRARY_EXPORT void flip_image_vertically(int width, int height, int components, unsigned char* pixels);
	LIBRARY_EXPORT renderer::Texture* load_default_texture(Image& image);

	//
	// image manipulation functions


//	bool LoadCubemap( const char ** filenames, unsigned int & texID, unsigned int flags, unsigned int * out_width = 0, unsigned int * out_height = 0 );
//	unsigned char * AllocImageFromFile( const char * filename, unsigned int * width, unsigned int * height, unsigned int * format, bool path_is_relative=true );

	LIBRARY_EXPORT Image load_from_memory(unsigned char* data, unsigned int data_size);
	LIBRARY_EXPORT unsigned char* load_image_from_memory(unsigned char* data, unsigned int dataSize, unsigned int* width, unsigned int* height, unsigned int* channels);
//	void save_image_to_file( const char * filename, unsigned int width, unsigned int height, unsigned int channels, unsigned char * pixels, int imageType );
	LIBRARY_EXPORT void free_image(unsigned char* pixels);

} // namespace image

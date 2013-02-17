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
#pragma once
#include "typedefs.h"
#include "color.hpp"



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
		F_WRAP = 8,		// wrap out of bounds UV coords
		F_CLAMP = 16,	// clamp out of bounds UV coords
		F_CUBEMAP = 32,	// load this texture as a cubemap
	}; // ImageFlags
	
	const unsigned int ERROR_TEXTURE_WIDTH = 128;
	const unsigned int ERROR_TEXTURE_HEIGHT = 128;


	void generate_checker_image( unsigned char * pixels, int width, int height, const Color & color1, const Color & color2 );
	void flip_image_vertically( int width, int height, int components, unsigned char * pixels );
	unsigned int load_default_texture();
	
	//
	// image manipulation functions
	
	bool load_image_from_file( const char * filename, unsigned int & texID, unsigned int flags, unsigned int * out_width = 0, unsigned int * out_height = 0 );
	
//	bool LoadCubemap( const char ** filenames, unsigned int & texID, unsigned int flags, unsigned int * out_width = 0, unsigned int * out_height = 0 );
//	unsigned char * AllocImageFromFile( const char * filename, unsigned int * width, unsigned int * height, unsigned int * format, bool path_is_relative=true );

	unsigned char * load_image_from_memory( unsigned char * data, unsigned int dataSize, unsigned int * width, unsigned int * height, unsigned int * channels );
//	void save_image_to_file( const char * filename, unsigned int width, unsigned int height, unsigned int channels, unsigned char * pixels, int imageType );
	void free_image( unsigned char * pixels );
	
	//
	// image <-> renderer interface
	
	void driver_release_image( unsigned int texture_id );
	void driver_upload_image2d( unsigned int & texID, unsigned int flags, unsigned int width, unsigned int height, unsigned int channels, unsigned char * pixels );
	
}; // namespace image
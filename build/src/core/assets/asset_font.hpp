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

#include "assets.hpp"
#include "stackstring.hpp"

namespace assets
{
	struct FontAsset : public Asset
	{
		unsigned short font_size;
		char * font_data;
		
		FontAsset();
		virtual void release();
	}; // FontAsset
	
	struct FontParameters : public AssetParameters
	{
		uint16_t point_size;
		
		FontParameters()
		{
			point_size = 0;
		} // FontParameters
	}; // FontParameters

	AssetLoadStatus font_load_callback( const char * path, FontAsset * config, const FontParameters & parameters );
	void font_construct_extension( StackString<MAX_PATH_SIZE> & extension );

	DECLARE_ASSET_LIBRARY_ACCESSOR(FontAsset, FontParameters, fonts);
}; // namespace assets
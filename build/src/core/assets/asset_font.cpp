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
#include "stackstring.hpp"
#include "assets.hpp"
#include "assets/asset_font.hpp"
#include "configloader.hpp"
#include "render_utilities.hpp"

namespace assets
{
	FontAsset::FontAsset()
	{

	}

	void FontAsset::release()
	{
	} // release

	AssetLoadStatus font_load_callback( const char * path, FontAsset * config, unsigned int flags )
	{
//		if ( util::json_load_with_callback( path, 0, config, true ) == util::ConfigLoad_Success )
//		{
//			return AssetLoad_Success;
//		}
		
		return AssetLoad_Failure;
	} // texture_load_callback
	
	
	void font_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = ".ttf";
	} // font_construct_extension

}; // namespace assets

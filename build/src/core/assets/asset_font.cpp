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

#include "font.hpp"
#include "kernel.hpp"

namespace assets
{
	Font::Font()
	{
		font_data = 0;
		font_size = 0;
	}

	void Font::release()
	{
		if ( font_data != 0 )
		{
			DEALLOC( this->font_data );
			this->font_size = 0;
		}
	} // release
	
	
	
	util::ConfigLoadStatus load_font_from_file( const Json::Value & root, void * data )
	{
		Font * font = (Font*)data;
		if (!font)
		{
			return util::ConfigLoad_Failure;
		}
		
		Json::Value point_size = root["point_size"];
		Json::Value font_file = root["file"];
		
		font->font_size = point_size.asInt();
		font->font_data = font::load_font_from_file( font_file.asString().c_str(), font->font_size, font->font_id );
		
		if (kernel::instance()->parameters().device_flags & kernel::DeviceSupportsRetinaDisplay)
		{
			font->font_size = font->font_size * 2;
		}
		
		if ( font->font_id != 0 )
		{
			return util::ConfigLoad_Success;
		}

		return util::ConfigLoad_Failure;
	} // load_font_from_file

	AssetLoadStatus font_load_callback( const char * path, Font * font, const AssetParameters & parameters )
	{
		if ( util::json_load_with_callback(path, load_font_from_file, font, true ) == util::ConfigLoad_Success )
		{		
			return AssetLoad_Success;
		}
		
		return AssetLoad_Failure;
	} // font_load_callback
	
	
	void font_construct_extension( StackString<MAX_PATH_SIZE> & extension )
	{
		extension = ".conf";
	} // font_construct_extension

}; // namespace assets

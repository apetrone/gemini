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
#include <runtime/configloader.h>
#include <core/stackstring.h>
#include <runtime/filesystem.h>

#include "assets.h"
#include "assets/asset_font.h"
//#include <renderer/render_utilities.h>
//#include <renderer/font.h>
#include "kernel.h"

#include <renderer/font.h>

namespace gemini
{
	namespace assets
	{
		Font::Font()
		{
			font_size = 0;		
			font_data = 0;
			handle = 0;
		}

		void Font::release()
		{
			if ( font_data != 0 )
			{
				MEMORY_DEALLOC(this->font_data, platform::memory::global_allocator());
				this->font_size = 0;
			}
		} // release
		
		
		
		char* load_font_from_file(const char* path, unsigned short point_size, font::Handle& font)
		{
			size_t font_data_size = 0;
			char * font_data = 0;
			font_data = core::filesystem::file_to_buffer( path, 0, &font_data_size );
			
			if ( font_data )
			{
	//			LOGV( "font data size: %i bytes\n", font_data_size );
				font = font::load_font_from_memory( font_data, font_data_size, point_size );
			}
			else
			{
				LOGE( "Unable to load font from file: '%s'\n", path );
				return 0;
			}
			
			return font_data;
		} // load_font_from_file
		
		
		
		core::util::ConfigLoadStatus load_font_from_file( const Json::Value & root, void * data )
		{
			Font* font = (Font*)data;
			if (!font)
			{
				return core::util::ConfigLoad_Failure;
			}
			
			Json::Value point_size = root["point_size"];
			Json::Value font_file = root["file"];
			
			font->font_size = point_size.asInt();
			font->font_data = load_font_from_file(font_file.asString().c_str(), font->font_size, font->handle);
			
			if (kernel::parameters().device_flags & kernel::DeviceSupportsRetinaDisplay)
			{
				font->font_size = font->font_size * 2;
			}
			
			if (font)
			{
				return core::util::ConfigLoad_Success;
			}

			return core::util::ConfigLoad_Failure;
		} // load_font_from_file

		AssetLoadStatus font_load_callback( const char * path, Font * font, const AssetParameters & parameters )
		{
			if ( core::util::json_load_with_callback(path, load_font_from_file, font, true ) == core::util::ConfigLoad_Success )
			{		
				return AssetLoad_Success;
			}
			
			return AssetLoad_Failure;
		} // font_load_callback
		
		
		void font_construct_extension( core::StackString<MAX_PATH_SIZE> & extension )
		{
			extension = ".conf";
		} // font_construct_extension

	} // namespace assets
} // namespace gemini
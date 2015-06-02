// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "assets/asset_animation.h"

namespace gemini
{
	namespace assets
	{
		Animation::Animation()
		{
		}

		void Animation::release()
		{
		} // release
		
		core::util::ConfigLoadStatus load_animation_from_file( const Json::Value & root, void * data )
		{
//			Font* font = (Font*)data;
//			if (!font)
//			{
//				return core::util::ConfigLoad_Failure;
//			}
//			
//			Json::Value point_size = root["point_size"];
//			Json::Value font_file = root["file"];
//			
//			font->font_size = point_size.asInt();
//			font->font_data = load_font_from_file(font_file.asString().c_str(), font->font_size, font->handle);
//			
//			if (kernel::parameters().device_flags & kernel::DeviceSupportsRetinaDisplay)
//			{
//				font->font_size = font->font_size * 2;
//			}
//			
//			if (font)
//			{
//				return core::util::ConfigLoad_Success;
//			}

			return core::util::ConfigLoad_Failure;
		} // load_animation_from_file

		AssetLoadStatus animation_load_callback(const char* path, Animation* animation, const AssetParameters& parameters)
		{
			if (core::util::json_load_with_callback(path, load_animation_from_file, animation, true) == core::util::ConfigLoad_Success)
			{		
				return AssetLoad_Success;
			}
			
			return AssetLoad_Failure;
		} // animation_load_callback
		
		
		void animation_construct_extension(core::StackString<MAX_PATH_SIZE>& extension)
		{
			extension = ".conf";
		} // animation_construct_extension

	} // namespace assets
} // namespace gemini
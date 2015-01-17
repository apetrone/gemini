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
#include "render_utilities.h"
#include <core/logging.h>


//#include "assets.h"

#include "renderstream.h"
#include "rqueue.h"

namespace gemini
{
	namespace render_utilities
	{
		//
		// misc sprite tools
		namespace sprite
		{
			void calc_tile_uvs( float * uvs, unsigned int x, unsigned int y, unsigned int sprite_width, unsigned int sprite_height, unsigned int sheet_width, unsigned int sheet_height )
			{
				// This assumes an Orthographic projection set with the origin in the upper left
				// upper left
				uvs[0] = x / (float)sheet_width;
				uvs[1] = y / (float)sheet_height;
				
				// lower left
				uvs[2] = x / (float)sheet_width;
				uvs[3] = (y+sprite_height) / (float)sheet_height;
				
				// lower right
				uvs[4] = (x+sprite_width) / (float)sheet_width;
				uvs[5] = (y+sprite_height) / (float)sheet_height;
				
				// upper right
				uvs[6] = (x+sprite_width) / (float)sheet_width;
				uvs[7] = y / (float)sheet_height;
			} // calc_tile_uvs
		}; // sprite
		
		void strip_shader_version(char* buffer, StackString<32>& version)
		{
			// remove preceding "#version" shader
			const char* pos = core::str::strstr(buffer, "#version");
			if (pos)
			{
				const char* end = pos;
				while(*end != '\n')
					++end;
				
				version._length = (end-pos);
				memcpy(&version[0], &buffer[(pos-buffer)], version._length);
				memset(&buffer[(pos-buffer)], ' ', (end-pos));
			}
		} // strip_shader_version
	} // mamespace render_utilities
} // namespace gemini

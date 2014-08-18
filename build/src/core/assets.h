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

#include <string>

#include <gemini/util/stackstring.h>
#include <gemini/platform.h>

#include <gemini/mathlib.h> // for glm
#include "color.h"

namespace assets
{
	// Asset utils
	enum AssetType
	{
		SoundAsset
	}; // AssetType
	
	
	// called to initialize default textures and other required resources.
	void startup();
	
	// purge all assets
	void purge();
	
	// purge all assets and reclaim unused memory
	void shutdown();
	
	// Given a relative path to an asset, tack on a platform-specific file extension
	void append_asset_extension( AssetType type, StackString< MAX_PATH_SIZE > & path );
	
	typedef unsigned int AssetID;
	
	struct Asset
	{
		assets::AssetID asset_id;
		virtual ~Asset() {}
		virtual void release() = 0;
		
		inline unsigned int Id() const
		{
			return asset_id;
		} // Id
	}; // Asset
	
	
	struct AssetParameters
	{
		
	}; // AssetParameters
	
	enum AssetLoadStatus
	{
		AssetLoad_Success = 0,
		AssetLoad_Failure = 1
	};
	
	// other shared types
//	typedef std::string ShaderString;
	typedef std::basic_string<char, std::char_traits<char>, GeminiAllocator<char> > ShaderString;
}; // namespace assets

#define IMPLEMENT_ASSET_LIBRARY_ACCESSOR( type, name )\
	type * _##name = 0;\
	type * name()\
	{\
		return _##name;\
	}
	
#define DECLARE_ASSET_LIBRARY_ACCESSOR( type, parameter_class, name )\
	typedef AssetLibrary<type, parameter_class> type##AssetLibrary;\
	type##AssetLibrary * name();


#include "assetlibrary.h"

#include "assets/asset_texture.h"
#include "assets/asset_shader.h"
#include "assets/asset_material.h"
#include "assets/asset_mesh.h"
#include "assets/asset_spriteconfig.h"
#include "assets/asset_emitter.h"
#include "assets/asset_font.h"

namespace assets
{
	unsigned int find_parameter_mask( ShaderString & name );
}; // namespace assets
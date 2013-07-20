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

#include "platform.hpp"
#include "stackstring.hpp"

#include "mathlib.h" // for glm
#include "color.hpp"
#include "renderer.hpp"

#include <string>

namespace assets
{
	// Asset utils
	enum AssetType
	{
		AssetUnknown = 0,
		TextureAsset,
		MaterialAsset,
		MeshAsset,
		ShaderAsset,
		SoundAsset,
	}; // AssetType
	
	
	// called to initialize default textures and other required resources.
	void startup();
	
	// purge all assets
	void purge();
	
	// purge all assets and reclaim unused memory
	void shutdown();
	
	// Given a relative path to an asset, convert it to an absolute path and tack on file extension
	// "sounds/handy" -> "<content_directory>/sounds/handy.<platform_extension>"
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
	
	enum AssetLoadStatus
	{
		AssetLoad_Success = 0,
		AssetLoad_Failure = 1
	};
	
	
	// other shared types
	typedef std::string ShaderString;
}; // namespace assets

#include "assetlibrary.hpp"

#include "assets/asset_texture.hpp"
#include "assets/asset_shader.hpp"
#include "assets/asset_material.hpp"
#include "assets/asset_mesh.hpp"

namespace assets
{
	
	
}; // namespace assets
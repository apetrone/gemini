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
	
	// Given a relative path to an asset, convert it to an absolute path and tack on file extension
	// "sounds/handy" -> "<content_directory>/sounds/handy.<platform_extension>"
	void construct_absolute_path_from_relative_path( AssetType type, StackString< MAX_PATH_SIZE > & path );
	
	
	// called to initialize default textures and other required resources.
	void startup();
	
	void shutdown();
	
	typedef unsigned int AssetID;
	
	struct Asset
	{
		assets::AssetID _asset_id;
		virtual ~Asset() {}
		virtual void release() = 0;
	}; // Asset
	
	
	
	// TODO: These may be better moved off into their own files and included here...
	struct Texture : public virtual Asset
	{
		char * path;
		unsigned int texture_id;
		unsigned int width;
		unsigned int height;
		unsigned int flags;
		
		virtual void release();
	};
	
	// load a texture from disk or cache. if reload_from_disk is false, cache is preferred
	Texture * load_texture( const char * path, unsigned int flags = 0, bool ignore_cache = false );
//	Texture * load_cubemap( const char * basename, unsigned int flags = 0, bool ignore_cache = false );
	
	
	
}; // namespace assets
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
#pragma once

#include <core/stackstring.h>
#include <core/mathlib.h> // for glm

#include <platform/platform.h>

#include <renderer/color.h>

namespace gemini
{
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
		void append_asset_extension( AssetType type, core::StackString< MAX_PATH_SIZE > & path );

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
	} // namespace assets
} // namespace gemini

#define IMPLEMENT_ASSET_LIBRARY_ACCESSOR( type, name )\
	type * _##name = 0;\
	type * name()\
	{\
		return _##name;\
	}

#define DECLARE_ASSET_LIBRARY_ACCESSOR( type, parameter_class, name )\
	typedef AssetLibrary<type, parameter_class> type##AssetLibrary;\
	type##AssetLibrary * name()


#include <runtime/assetlibrary.h>

#include <runtime/assets/asset_texture.h>
#include <runtime/assets/asset_shader.h>
#include <runtime/assets/asset_material.h>
#include <runtime/assets/asset_mesh.h>
#include <runtime/assets/asset_emitter.h>
#include <runtime/assets/asset_font.h>
#include <runtime/assets/asset_sound.h>

namespace gemini
{
	namespace assets
	{
		unsigned int find_parameter_mask(::renderer::ShaderString& name);
	} // namespace assets
} // namespace gemini

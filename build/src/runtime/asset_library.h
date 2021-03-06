// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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


#include <core/hashset.h>
#include <core/logging.h>
#include <core/typedefs.h>
#include <core/stackstring.h>

#include <platform/platform.h>

#include <runtime/asset_handle.h>

#if 0
// Design Goals
// load asset via path
// asset should be cached
// assets should be re-loadable
// assets live in runtime, so anything at runtime or above can access them.
// assets should have external create/destroy callbacks
//	- this allows a subsystem to customize that
// assets should be returned as handles.
//	- this allows handles to be invalid, but not return raw pointers.
//	- this also allows handles to be lazily loaded or streamed
// should allow a default asset of sorts -- optional. If a named asset cannot be found.
// lookup asset pointer by asset handle (id)
// purge all assets
// allow insertion of arbitrary assets (creating default material/texture, for example)
// allow a prefix-path to be prepended to the search path
// allow asset parameters (only texture uses this)
// specify an allocator which can be passed in to create/destroy functions
#endif

namespace gemini
{
	enum AssetLoadStatus
	{
		// The asset is loaded and ready to use.
		AssetLoad_Success,

		// The asset could not be loaded.
		AssetLoad_Failure
	}; // AssetLoadStatus


	template <class T, class D>
	class AssetLibrary2
	{
	public:
		typedef uint16_t HandleIndex;
		typedef HashSet<platform::PathString, HandleIndex> HandleIndexByName;
		typedef T AssetClass;

		struct LoadState
		{
			Allocator* allocator;

			// asset instance
			T* asset;

			// path of the current asset on disk
			platform::PathString asset_uri;
		}; // AssetLoadState

		Allocator& allocator;
		Array<T*> assets;
		T* default_asset_pointer;
		HandleIndexByName handle_by_name;
		platform::PathString prefix_uri;

	private:
		D* instance()
		{
			return static_cast<D*>(this);
		}

		D& instance_reference()
		{
			return *instance();
		}

	public:

		AssetLibrary2(gemini::Allocator& asset_allocator)
			: allocator(asset_allocator)
			, assets(asset_allocator)
			, default_asset_pointer(nullptr)
			, handle_by_name(asset_allocator)
		{
		}

		AssetLibrary2 operator=(const AssetLibrary2& other) = delete;
		AssetLibrary2 operator=(const AssetLibrary2&& other) = delete;
		AssetLibrary2(const AssetLibrary2& other) = delete;

		virtual ~AssetLibrary2()
		{
			purge();
		}

		T* create(Allocator& allocator, void* parameters)
		{
			LoadState state;
			state.allocator = &allocator;
			state.asset = nullptr;
			instance_reference().create_asset(state, parameters);
			return state.asset;
		}

		void destroy(T* asset)
		{
			LoadState state;
			state.allocator = &allocator;
			state.asset = asset;
			instance_reference().destroy_asset(state);
		}

		void default_asset(T* asset)
		{
			default_asset_pointer = asset;
		}

		T* default_asset() const
		{
			return default_asset_pointer;
		}

		bool handle_is_valid(AssetHandle handle)
		{
			return (handle.index > 0) && (handle.index <= assets.size());
		} // handle_is_valid

		AssetHandle load(const char* relative_path, bool ignore_cache = false, void* parameters = nullptr)
		{
			// 1. Check to see if the asset is already loaded...
			platform::PathString fullpath = prefix_uri;
			fullpath.append(relative_path);
			uint8_t asset_is_new = 1;
			HandleIndex handle_index;
			handle_index = 0;

			platform::path::normalize(&fullpath[0]);
			if (handle_by_name.has_key(fullpath()))
			{
				HandleIndex index_to_check = handle_by_name[fullpath()];
				AssetHandle asset_handle;
				asset_handle.index = index_to_check;
				T* asset = lookup(asset_handle);

				// Some asset types require parameters to validate that they
				// are indeed the same asset.
				if (instance_reference().is_same_asset(asset, parameters))
				{
					asset_is_new = 0;
					handle_index = index_to_check;

					// An entry exists for it: return it.
					if (!ignore_cache)
					{
						AssetHandle handle;
						handle.index = handle_index;
						return handle;
					}
				}
			}

			LoadState load_state;
			load_state.asset = nullptr;
			load_state.allocator = &allocator;
			load_state.asset_uri = fullpath;

			instance_reference().create_asset(load_state, parameters);
			AssetLoadStatus load_result = instance_reference().load_asset(load_state, fullpath, parameters);
			if (load_result == AssetLoad_Success)
			{
				AssetHandle handle = InvalidAssetHandle;
				if (asset_is_new)
				{
					handle = take_ownership(fullpath, load_state.asset, false);
				}
				else
				{
					// swap out the old with the new.
					assert(handle_index > 0);

					T* old_asset = assets[static_cast<int>(handle_index - 1)];
					assets[handle_index - 1] = load_state.asset;

					// delete the old asset
					load_state.asset = old_asset;
					instance_reference().destroy_asset(load_state);
				}

				return handle;
			}
			else
			{
				// free created asset
				instance_reference().destroy_asset(load_state);
				LOGW("FAILED to load asset [%s]\n", relative_path);
			}

			AssetHandle handle = InvalidAssetHandle;
			handle.index = 0;
			return handle;
		} // load

		T* lookup(AssetHandle handle)
		{
			if (!handle_is_valid(handle))
			{
				return default_asset();
			}

			return assets[handle.index - 1];
		} // lookup

		void purge()
		{
			for (size_t index = 0; index < assets.size(); ++index)
			{
				LoadState load_state;
				load_state.allocator = &allocator;
				load_state.asset = assets[index];
				instance_reference().destroy_asset(load_state);
			}
			assets.clear();
			handle_by_name.clear();
		} // purge

		void prefix_path(const platform::PathString& prefix)
		{
			prefix_uri = prefix;

			// ensure prefix_path has a trailing slash
			prefix_uri.strip_trailing(PATH_SEPARATOR);
			prefix_uri.append(PATH_SEPARATOR_STRING);
		} // prefix_path

		const platform::PathString& prefix_path() const
		{
			return prefix_uri;
		} // prefix_path

		AssetHandle take_ownership(const platform::PathString& path, T* asset, bool verify_unique = true)
		{
			AssetHandle handle;

			platform::PathString normalized_path = path;
			normalized_path.normalize(PATH_SEPARATOR);

			if (!verify_unique || !handle_by_name.has_key(normalized_path))
			{
				assert(assets.size() < USHRT_MAX);
				uint16_t index = static_cast<uint16_t>(assets.size());
				handle_by_name[normalized_path] = index + 1;
				assets.push_back(asset);

				handle.index = (index + 1);
				return handle;
			}

			handle.index = 0;
			return handle;
		} // take_ownership
	}; // AssetLibrary2
} // namespace gemini

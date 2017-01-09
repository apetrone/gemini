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

#include <map>
#include <list>

#include <core/logging.h>
#include <core/typedefs.h>
#include <core/stackstring.h>
#include <platform/platform.h>

#include "assets.h"

namespace gemini
{
	namespace assets
	{
		template <class T>
		struct AssetLoadState
		{
			gemini::Allocator* allocator;
			T* asset;
		}; // AssetLoadState

		template <class AssetClass, class AssetParameterClass>
		AssetLoadStatus asset_default_create(const char* path, AssetLoadState<AssetClass>& load_state, const AssetParameterClass& parameters)
		{
			load_state.asset = MEMORY2_NEW(*load_state.allocator, AssetClass)(load_state.allocator);
			return AssetLoad_Success;
		}

		template <class AssetClass>
		void asset_default_destroy(AssetLoadState<AssetClass>& load_state)
		{
			MEMORY2_DELETE(*load_state.allocator, load_state.asset);
		}

		template <class AssetClass, class AssetParameterClass = AssetParameters>
		class AssetLibrary
		{
			typedef AssetLoadStatus (*asset_create_fn)(const char* path, AssetLoadState<AssetClass>& load_state, const AssetParameterClass& parameters);
			typedef void (*asset_destroy_fn) (AssetLoadState<AssetClass>& load_state);
			typedef void (*AssetConstructExtension)( core::StackString<MAX_PATH_SIZE> & path );
			typedef void (*AssetIterator)( AssetClass * asset, void * userdata );

			typedef std::map<std::string, AssetClass*> AssetHashTable;
			typedef typename AssetHashTable::iterator AssetHashTableIterator;
			typedef std::list<AssetClass*> AssetList;

			unsigned int total_assets;
			asset_create_fn asset_create;
			asset_destroy_fn asset_destroy;
			AssetConstructExtension construct_extension_callback;
			AssetHashTable asset_by_name;
			AssetList asset_list;
			AssetClass * default_asset;


			gemini::Allocator& allocator;

			platform::PathString prefix_path;

		public:

			AssetLibrary(gemini::Allocator& allocator,
							AssetConstructExtension extension_callback,
							asset_create_fn create_asset = asset_default_create<AssetClass, AssetParameterClass>,
							asset_destroy_fn destroy_asset = asset_default_destroy<AssetClass>)
				: allocator(allocator)
				, asset_create(create_asset)
				, asset_destroy(destroy_asset)
			{
				// If you hit either of these, assets lifecycle cannot be
				// managed properly.
				assert(asset_create != nullptr);
				assert(asset_destroy != nullptr);

				construct_extension_callback = extension_callback;
				assert( construct_extension_callback != 0 );

				total_assets = 0;
				default_asset = 0;
			} // AssetLibrary

			~AssetLibrary()
			{
				release_and_purge();
			}

			unsigned int total_asset_count() const { return total_assets; }

			void for_each( AssetIterator iterator, void * userdata )
			{
				AssetClass * asset = 0;
				typename AssetList::iterator it = asset_list.begin();
				typename AssetList::iterator end = asset_list.end();

				for( ; it != end; ++it )
				{
					asset = (*it);
					iterator( asset, userdata );
				}
			} // for_each

			void set_prefix_path(const platform::PathString& prefix)
			{
				prefix_path = prefix;

				// ensure prefix_path has a trailing slash
				prefix_path.strip_trailing(PATH_SEPARATOR);
				prefix_path.append(PATH_SEPARATOR_STRING);
			} // set_prefix_path

			const platform::PathString& get_prefix_path() const
			{
				return prefix_path;
			} // get_prefix_path

			// providing stubs for these functions
			void construct_extension( core::StackString<MAX_PATH_SIZE> & extension )
			{
				construct_extension_callback(extension);
			} // construct_extension

			void append_extension( core::StackString<MAX_PATH_SIZE> & path )
			{
				core::StackString<MAX_PATH_SIZE> extension;
				this->construct_extension(extension);
				path.append( extension() );
			}

			AssetClass * load_from_path(const char* path, const AssetParameterClass & parameters = AssetParameterClass(), bool ignore_cache = false)
			{
				// This should handle the following cases:
				// 1) Asset is loaded, obey cache and return loaded asset
				// 2) Asset is loaded. User requests a reload of asset by ignoring the cache.
				// 3) Asset is not loaded yet. Load it.
				AssetClass* asset = 0;
				core::StackString<MAX_PATH_SIZE> fullpath = prefix_path;
				fullpath.append(path);
				int load_result = 0;
				int asset_is_new = 0;

				// is this asset already loaded?
				AssetHashTableIterator iter = asset_by_name.find(path);
				if (iter != asset_by_name.end())
				{
					asset = iter->second;

					if (!ignore_cache)
					{
						// case 1
	//					LOGV( "asset (%s) already loaded. returning from cache\n", path );
						return asset;
					}
				}

				// append the proper extension for this platform
				this->append_extension(fullpath);


				if (!asset)
				{
					// case 3
					asset_is_new = 1;
				}

				// case 2 && 3
				AssetLoadState<AssetClass> load_state;
				load_state.asset = asset;
				load_state.allocator = &allocator;
				load_result = asset_create(fullpath(), load_state, parameters);
				if (load_result != AssetLoad_Failure)
				{
					if (asset_is_new)
					{
						take_ownership(path, load_state.asset);
					}

					//LOGV("loaded asset \"%s\", asset_id = %i\n", fullpath(), asset->Id());
					return load_state.asset;
				}
				else
				{
					LOGV( "asset (%s) loading failed!\n", path );
				}

				return default_asset;
			} // load_from_path


			// take ownership of this asset; should be managed by this class from here on
			void take_ownership(const char* path, AssetClass * asset)
			{
				if (!this->find_with_path(path))
				{
					//assert(0); // TODO@apetrone (assets) fix asset ids
					//asset->asset_id = total_assets++;
					asset_list.push_back(asset);

					asset_by_name[path] = asset;
				}
			} // take_ownership

			AssetClass* find_with_path(const char* path)
			{
				AssetClass * asset = 0;

				AssetHashTableIterator iter = asset_by_name.find(path);
				if (iter != asset_by_name.end())
				{
					asset = iter->second;
					return asset;
				}

				return 0;
			} // find

			AssetClass * find_with_id( assets::AssetID id )
			{
				AssetClass * asset = 0;
				typename AssetList::iterator it = asset_list.begin();
				typename AssetList::iterator end = asset_list.end();

				for( ; it != end; ++it )
				{
					asset = (*it);
					if ( asset->asset_id == id )
					{
						return asset;
					}
				}

				return 0;
			} // find_with_id

			// release and purge all assets
			void release_and_purge()
			{
				typename AssetList::iterator it = asset_list.begin();
				typename AssetList::iterator end = asset_list.end();

				AssetClass * asset;
				for( ; it != end; ++it )
				{
					asset = (*it);
					AssetLoadState<AssetClass> load_state;
					load_state.asset = asset;
					load_state.allocator = &allocator;
					asset_destroy(load_state);
				}

				total_assets = 0;

				asset_by_name.clear();
			} // release_and_purge

			void set_default( AssetClass * asset )
			{
				default_asset = asset;
			} // set_default

			AssetClass * get_default() const
			{
				return default_asset;
			} // get_default
		}; // AssetLibrary
	} // namespace assets
} // namespace gemini

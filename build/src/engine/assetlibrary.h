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

#include "assets.h"

namespace gemini
{
	namespace assets
	{
		template <class AssetClass, class AssetParameterClass = AssetParameters>
		class AssetLibrary
		{
			typedef AssetLoadStatus (*AssetLoadCallback)( const char * path, AssetClass * asset, const AssetParameterClass & parameters );
			typedef void (*AssetConstructExtension)( core::StackString<MAX_PATH_SIZE> & path );
			typedef void (*AssetIterator)( AssetClass * asset, void * userdata );

			typedef std::map<std::string, AssetClass*> AssetHashTable;
			typedef typename AssetHashTable::iterator AssetHashTableIterator;
			typedef std::list<AssetClass*> AssetList;

			unsigned int total_assets;
			AssetLoadCallback load_callback;
			AssetConstructExtension construct_extension_callback;
			AssetHashTable asset_by_name;
			AssetList asset_list;
			AssetClass * default_asset;

		public:

			AssetLibrary( AssetLoadCallback callback, AssetConstructExtension extension_callback )
			{
				load_callback = callback;
				assert( load_callback != 0 );

				construct_extension_callback = extension_callback;
				assert( construct_extension_callback != 0 );

				total_assets = 0;
				default_asset = 0;
			} // AssetLibrary

			~AssetLibrary()
			{
				release_and_purge();
			}

			AssetClass * allocate_asset() { return MEMORY_NEW(AssetClass, core::memory::global_allocator()); }
			void deallocate_asset( AssetClass * asset ) { MEMORY_DELETE(asset, core::memory::global_allocator()); }
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

			// providing stubs for these functions
			AssetLoadStatus load_with_callback( const char * path, AssetClass * asset, const AssetParameterClass & parameters )
			{
				if ( !load_callback )
				{
					return AssetLoad_Failure;
				}

				return load_callback( path, asset, parameters );
			} // load_with_callback

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
				AssetClass * asset = 0;
				core::StackString<MAX_PATH_SIZE> fullpath = path;
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
					asset = allocate_asset();
					asset_is_new = 1;
				}

				// case 2 && 3
				load_result = load_with_callback(fullpath(), asset, parameters);
				if (load_result != AssetLoad_Failure)
				{
					if (asset_is_new)
					{
						core::StackString<MAX_PATH_SIZE> store_path = path;
						take_ownership(store_path(), asset);
					}

					LOGV("loaded asset \"%s\", asset_id = %i\n", fullpath(), asset->Id());
					return asset;
				}
				else
				{
					if ( asset_is_new )
					{
						deallocate_asset( asset );
					}
					LOGV( "asset (%s) loading failed!\n", path );
				}

				return default_asset;
			} // load_from_path


			// take ownership of this asset; should be managed by this class from here on
			void take_ownership(const char* path, AssetClass * asset)
			{
				if (!this->find_with_path(path))
				{
					asset->asset_id = total_assets++;
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
					asset->release();
					deallocate_asset( asset );
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

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
#include <map>

#include "typedefs.h"
#include "assets.h"
#include <list>
#include <slim/xlog.h>

namespace assets
{
	


	template <class AssetClass, class AssetParameterClass = AssetParameters>
	class AssetLibrary
	{
		typedef AssetLoadStatus (*AssetLoadCallback)( const char * path, AssetClass * asset, const AssetParameterClass & parameters );
		typedef void (*AssetConstructExtension)( StackString<MAX_PATH_SIZE> & path );
		typedef void (*AssetIterator)( AssetClass * asset, void * userdata );
		
		typedef std::map<std::string, AssetClass*> AssetHashTable;
		typedef typename AssetHashTable::iterator AssetHashTableIterator;
		typedef std::list<AssetClass*, GeminiAllocator<AssetClass*> > AssetList;
		
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
	
		AssetClass * allocate_asset() { return CREATE(AssetClass); }
		void deallocate_asset( AssetClass * asset ) { DESTROY(AssetClass, asset); }
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
		
		void construct_extension( StackString<MAX_PATH_SIZE> & extension )
		{
			construct_extension_callback(extension);
		} // construct_extension
		
		void append_extension( StackString<MAX_PATH_SIZE> & path )
		{
			StackString<MAX_PATH_SIZE> extension;
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
			StackString<MAX_PATH_SIZE> fullpath = path;
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
					StackString<MAX_PATH_SIZE> store_path = path;
					take_ownership(store_path(), asset);
				}
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

}; // namespace assets
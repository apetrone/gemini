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

#include "typedefs.h"
#include "assets.hpp"
#include "hashtable.hpp"
#include <list>
#include "log.h"

#define LOGDEBUG LOGV
//#define LOGDEBUG

namespace assets
{
	enum AssetLoadStatus
	{
		AssetLoad_Success = 0,
		AssetLoad_Failure = 1
	};
	
	// a function that knows how to load in an asset

	
	template <class AssetClass, AssetType type>
	class AssetLibrary
	{
		typedef AssetLoadStatus (*AssetLoadCallback)( const char * path, AssetClass * asset, unsigned int flags );
		typedef void (*AssetIterator)( AssetClass * asset, void * userdata );
		typedef HashTable<AssetClass*> AssetHashTable;
		typedef std::list<AssetClass*> AssetList;
		
		unsigned int total_assets;
		AssetLoadCallback load_callback;
		AssetHashTable asset_by_name;
		AssetList asset_list;
	public:

		
		AssetLibrary( AssetLoadCallback callback )
		{
			load_callback = callback;
			assert( load_callback != 0 );
			total_assets = 0;
		} // AssetLibrary
	
		AssetType asset_type() const { return type; }
		
		AssetClass * allocate_asset() { return ALLOC(AssetClass); }
		void deallocate_asset( AssetClass * asset ) { DEALLOC(AssetClass, asset); }
		
		void foreach_asset( AssetIterator iterator, void * userdata )
		{
			Asset * asset = 0;
			typename AssetList::iterator it = asset_list.begin();
			typename AssetList::iterator end = asset_list.end();
			
			for( ; it != end; ++it )
			{
				asset = (*it);
				iterator( asset, userdata );
			}
		} // foreach_asset
	
		// providing stubs for these functions
	
		AssetLoadStatus load_with_callback( const char * path, AssetClass * asset, unsigned int flags )
		{
			if ( !load_callback )
			{
				return AssetLoad_Failure;
			}
		
			return load_callback( path, asset, flags );
		} // load_with_callback
		

		AssetClass * load_from_path( const char * path, unsigned int flags = 0, bool ignore_cache = false )
		{
			// This should handle the following cases:
			// 1) Asset is loaded, obey cache and return loaded asset
			// 2) Asset is loaded. User requests a reload of asset by ignoring the cache.
			// 3) Asset is not loaded yet. Load it.
			AssetClass * asset = 0;
			StackString< MAX_PATH_SIZE > fullpath = path;
			int load_result = 0;
			int asset_is_new = 0;
			
			// is this asset already loaded?
			if ( asset_by_name.contains( path ) )
			{
				asset = asset_by_name.get( path );
				
				if ( !ignore_cache )
				{
					// case 1
					LOGDEBUG( "asset (%s) already loaded. returning from cache\n", path );
					return asset;
				}
			}
			
			// append the proper extension for this platform
			assets::append_asset_extension( asset_type(), fullpath );
			
			if ( !asset )
			{
				// case 3
				asset = allocate_asset();
				asset_is_new = 1;
			}
			
			// case 2 && 3
			load_result = load_with_callback( fullpath(), asset, flags );
			if ( load_result != AssetLoad_Failure )
			{
				if ( asset_is_new )
				{
					take_ownership( path, asset );
				}
				return asset;
			}
			else
			{
				if ( asset_is_new )
				{
					deallocate_asset( asset );
				}
				LOGDEBUG( "asset (%s) loading failed!\n", path );
			}
			
			return 0;
		} // load_from_path
		
		
		// take ownership of this asset; should be managed by this class from here on
		void take_ownership( const char * path, AssetClass * asset )
		{
			asset->asset_id = total_assets++;
			asset_list.push_back( asset );
			asset_by_name.set( path, asset );
		} // take_ownership
		
		AssetClass * find_with_path( const char * path )
		{
			Asset * asset = 0;
			if ( asset_by_name.contains( path ) )
			{
				asset = asset_by_name.get( path );
				return asset;
			}
			
			return 0;
		} // find
		
		AssetClass * find_with_id( assets::AssetID id )
		{
			Asset * asset = 0;
			typename AssetList::iterator it = asset_list.begin();
			typename AssetList::iterator end = asset_list.end();
			
			for( ; it != end; ++it )
			{
				asset = (*it);
				if ( asset->asset_id == id )
					return asset;
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
			asset_by_name.purge();
		} // release_and_purge
	}; // AssetLibrary

}; // namespace assets
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

#include "assets.hpp"
#include "memory.hpp"

namespace assets
{
	enum AssetLoadStatus
	{
		Success = 0,
		Failure = 1
	};
	
	// a function that knows how to load in an asset

	
	template <class AssetClass, AssetType type>
	class AssetLibrary
	{
		typedef AssetLoadStatus (*AssetLoadCallback)( const char * path, AssetClass * asset, unsigned int flags );
		typedef void (*AssetIterator)( AssetClass * asset, void * userdata );
		
		unsigned int total_assets;
		AssetLoadCallback load_callback;
	public:

		
		AssetLibrary( AssetLoadCallback callback )
		{
			load_callback = callback;
			total_assets = 0;
		} // AssetLibrary
	
		AssetType asset_type() const { return type; }
		
		AssetClass * allocate_asset() { return ALLOC(AssetClass); }
		void deallocate_asset( AssetClass * asset ) { DEALLOC(AssetClass, asset); }
		
		void foreach_asset( AssetIterator iterator, void * userdata )
		{
			
		} // foreach_asset
	
		// providing stubs for these functions
	
		AssetLoadStatus load_with_callback( const char * path, AssetClass * asset, unsigned int flags )
		{
			return Failure;
		} // load_with_callback
		

		AssetClass * load_from_path( const char * path, unsigned int flags = 0, bool ignore_cache = false )
		{
			// This should handle the following cases:
			// 1) Asset is loaded, obey cache and return loaded asset
			// 2) Asset is loaded. User requests a reload of asset by ignoring the cache.
			// 3) Asset is not loaded yet. Load it.
			return 0;
		} // load_from_path
		
		
		// take ownership of this asset; should be managed by this class from here on
		void take_ownership( const char * path, AssetClass * asset )
		{
		} // take_ownership
		
		AssetClass * find_with_path( const char * path )
		{
			return 0;
		} // find
		
		AssetClass * find_with_id( assets::AssetID id )
		{
			return 0;
		} // find_with_id
		
		// release and purge all assets
		void release_and_purge()
		{
			
		} // release_and_purge
	}; // AssetLibrary

}; // namespace assets
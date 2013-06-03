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
#include "typedefs.h"
#include "log.h"
#include "tiledloader.hpp"

util::ConfigLoadStatus tiled_map_loader( const Json::Value & root, void * data )
{
	TiledMap * map = (TiledMap*)data;
	
	map->width = root["width"].asUInt();
	map->height = root["height"].asUInt();
	LOGV( "map dimensions: %i x %i\n", map->width, map->height );
	
	map->tile_width = root["tilewidth"].asUInt();
	map->tile_height = root["tileheight"].asUInt();
	LOGV( "tile dimensions: %i x %i\n", map->tile_width, map->tile_height );
	
	
	
	// load tile sets
	map->tilelist.tile_count = 0;
	Json::Value tilesets_group = root["tilesets"];
	map->tileset_count = tilesets_group.size();
	map->tilesets = CREATE_ARRAY( TileSet, map->tileset_count );
	
	Json::ValueIterator tileset_iterator = tilesets_group.begin();
	for( int tileset_id = 0; tileset_id < map->tileset_count; ++tileset_id, ++tileset_iterator )
	{
		TileSet * tileset = &map->tilesets[ tileset_id ];
		tileset->id = tileset_id;
		
		Json::Value key = tileset_iterator.key();
		Json::Value value = (*tileset_iterator);
		
		// load this tileset
		Json::Value name = value["name"];
		tileset->name = name.asString().c_str();
		LOGV( "Loading tileset '%s'\n", tileset->name() );
		
		Json::Value firstgid = value["firstgid"];
		tileset->firstgid = firstgid.asUInt();
		
		Json::Value image = value["image"];
		StackString<MAX_PATH_SIZE> image_path = image.asString().c_str();
		LOGV( "Tileset image: %s\n", image_path() );
		image_path.remove_extension();
		
		LOGV( "image: %s\n", image_path.basename() );
		StackString<MAX_PATH_SIZE> material_path = "materials/";
		material_path.append( image_path.basename() );
		
		LOGV( "material path: %s\n", material_path() );
		tileset->material = assets::load_material( material_path() );
		
		if ( tileset->material->parameter_by_name("diffusemap") )
		{
			LOGV( "material '%s' -> texture_id = %i\n", tileset->material->name.c_str(), tileset->material->parameter_by_name("diffusemap")->intValue );
		}
			
		
		tileset->imagewidth = value["imagewidth"].asInt();
		tileset->imageheight = value["imageheight"].asInt();
		LOGV( "tileset dimensions: %i %i\n", tileset->imagewidth, tileset->imageheight );
		
		map->tilelist.tile_count += map->count_tiles_in_set( tileset );
	}
	
	// now that all tile sets are loaded; let's create our tile list with all possible tiles
	map->tilelist.tile_count += 1;
	LOGV( "total # of tiles needed in list: %i\n", map->tilelist.tile_count );
	map->tilelist.tiles = CREATE_ARRAY( Tile, map->tilelist.tile_count );
	for( unsigned int i = 0; i < map->tileset_count; ++i )
	{
		TileSet * set = &map->tilesets[ i ];
		map->tilelist.create_tiles( set, map->tile_width, map->tile_height );
	}
	
	Json::Value layers = root["layers"];
	LOGV( "# layers: %i\n", layers.size() );
	
	map->layer_count = layers.size();
	map->layers = CREATE_ARRAY( TiledMapLayer, map->layer_count );
	
	// load tile layers
	Json::ValueIterator layer_iterator = layers.begin();
	for( int layer_id = 0; layer_id < map->layer_count && layer_iterator != layers.end(); ++layer_id, ++layer_iterator )
	{
		TiledMapLayer * tmlayer = &map->layers[ layer_id ];
		Json::Value key = layer_iterator.key();
		Json::Value value = (*layer_iterator);
		
		// load layer name
		Json::Value name = value["name"];
		unsigned int name_size = name.asString().size();
		tmlayer->name = (char*)ALLOC( name_size+1 );
		memset( tmlayer->name, 0, name_size+1 );
		xstr_ncpy( tmlayer->name, name.asString().c_str(), name_size );
		LOGV( "loading layer: %s\n", tmlayer->name );
		
		// load layer data
		Json::Value data = value["data"];
		unsigned int data_len = data.size();
		LOGV( "data_len: %i\n", data_len );
		tmlayer->layer_data = (unsigned char*)ALLOC( data_len );
		for( unsigned int i = 0; i < data_len; ++i )
		{
			// incoming data from tiled is a 1-based GID index.
			// a value of 0 indicates an empty tile in the layer
			tmlayer->layer_data[i] = (unsigned char)data[i].asInt();
		}
	}
	
	
	return util::ConfigLoad_Success;
} // tiled_map_loader


//
// TileSet
void TileSet::calc_tile_uvs( float * uvs, unsigned int x, unsigned int y, unsigned int sprite_width, unsigned int sprite_height, unsigned int sheet_width, unsigned int sheet_height )
{
	// This assumes an Orthographic projection set with the origin in the upper left
	// upper left
	uvs[0] = x / (float)sheet_width;
	uvs[1] = y / (float)sheet_height;
	
	// lower left
	uvs[2] = x / (float)sheet_width;
	uvs[3] = (y+sprite_height) / (float)sheet_height;
	
	// lower right
	uvs[4] = (x+sprite_width) / (float)sheet_width;
	uvs[5] = (y+sprite_height) / (float)sheet_height;
	
	// upper right
	uvs[6] = (x+sprite_width) / (float)sheet_width;
	uvs[7] = y / (float)sheet_height;
} // calc_tile_uvs

//
// TileList

TileList::TileList()
{
	tiles = 0;
	tile_count = 0;
}

TileList::~TileList()
{
	DESTROY_ARRAY( Tile, tiles, tile_count );
}

void TileList::create_tiles( TileSet * set, unsigned int tile_width, unsigned int tile_height )
{
	unsigned int num_columns = (set->imagewidth / tile_width);
	unsigned int num_rows = (set->imageheight / tile_height);
	
	unsigned int tile_id = 0;
	for( unsigned int y = 0; y < num_rows; ++y )
	{
		for( unsigned int x = 0; x < num_columns; ++x, ++tile_id )
		{
			Tile * tile = &tiles[ (set->firstgid + tile_id)-1 ];
			tile->id = (set->firstgid + tile_id)-1;
			tile->tileset_id = set->id;
			
			set->calc_tile_uvs( tile->quad_uvs, (x*tile_width), (y*tile_height), tile_width, tile_height, set->imagewidth, set->imageheight );
		}
	}
} // create_tiles


//
// TiledMapLayer
TiledMapLayer::TiledMapLayer()
{
	layer_data = 0;
	name = 0;
}

TiledMapLayer::~TiledMapLayer()
{
	DEALLOC( name );
	DEALLOC( layer_data );
}


//
// TiledMap

TiledMap::TiledMap()
{
	layers = 0;
	layer_count = 0;
	width = 0;
	height = 0;
	
	tileset_count = 0;
	tilesets = 0;
}

TiledMap::~TiledMap()
{
	DESTROY_ARRAY( TiledMapLayer, layers, layer_count );
	DESTROY_ARRAY( TileSet, tilesets, tileset_count );
}

TileSet * TiledMap::tileset_for_gid( unsigned int gid )
{
	for( int i = 0; i < tileset_count; ++i )
	{
		TileSet * set = &tilesets[ i ];
		unsigned int max_tiles = (set->imagewidth / this->tile_width) * (set->imageheight / this->tile_height);
		
		// if the gid is within this tileset's range, we've found it
		if ( gid >= set->firstgid && gid <= (set->firstgid + max_tiles) )
		{
			return set;
		}
	}
	
	return 0;
} // tileset_for_gid

unsigned int TiledMap::count_tiles_in_set( TileSet * set ) const
{
	if ( !set )
	{
		return 0;
	}
	
	return (set->imagewidth / this->tile_width) * (set->imageheight / this->tile_height);
} // count_tiles_in_set

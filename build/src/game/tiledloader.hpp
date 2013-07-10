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
#include <vector>

#include "memory.hpp"
#include "stackstring.hpp"
#include "assets.hpp"

#include "configloader.hpp"
util::ConfigLoadStatus tiled_map_loader( const Json::Value & root, void * data );

struct Tile
{
	unsigned short id;
	unsigned short tileset_id;
	float quad_uvs[8];
}; // Tile

struct TileSet
{
	StackString<64> name;
	unsigned int id;
	unsigned int firstgid;
	
	// TODO: I don't want to keep duplicate data around, but there's no way to get this data
	// directly from a material at the present.
	unsigned short imagewidth;
	unsigned short imageheight;
	
	assets::Material * material;	
}; // TileSet

struct TileList
{
	unsigned int tile_count;
	Tile * tiles;
	
	TileList();
	~TileList();
	
	void create_tiles( TileSet * set, unsigned int tile_width, unsigned int tile_height );
}; // TileList

struct TiledMapLayer
{
	char * name;
	
	unsigned char * layer_data;
	
	
	TiledMapLayer();
	~TiledMapLayer();
}; // TiledMapLayer

struct TiledMap
{
	unsigned int width;
	unsigned int height;
	
	unsigned int tile_width;
	unsigned int tile_height;
	
	unsigned int layer_count;
	TiledMapLayer * layers;
	
	unsigned int tileset_count;
	TileSet * tilesets;
	
	
	TileList tilelist;
	
	TiledMap();
	~TiledMap();
	
	
	TileSet * tileset_for_gid( unsigned int gid );
	unsigned int count_tiles_in_set( TileSet * set ) const;
}; // TiledMap


namespace sprite
{
	void calc_tile_uvs( float * uvs, unsigned int x, unsigned int y, unsigned int sprite_width, unsigned int sprite_height, unsigned int sheet_width, unsigned int sheet_height );
}; // namespace sprite
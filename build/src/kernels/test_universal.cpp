// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

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
#include "kernel.hpp"
#include <stdio.h>
#include "renderer.hpp"
#include "audio.hpp"
#include "input.hpp"
#include "log.h"
//#include <squirrel.h>
#include "filesystem.hpp"
#include "game/menu.hpp"
#include "mathlib.h"
#include "assets.hpp"
#include "camera.hpp"
#include "renderstream.hpp"
#include "font.hpp"
#include "render_utilities.hpp"
#include "tiledloader.hpp"
#include "screencontrol.hpp"
#include "map_event.hpp"

#define TEST_2D 0
#define TEST_FONT 1

glm::mat4 objectMatrix;

const float BULLET_SPEED = 500;
const float TEST_SIZE = 256;

void add_sprite_to_stream( renderer::VertexStream & vb, int x, int y, int width, int height, const Color & color, float * texcoords );

void world_to_screen( float & wx, float & wy, float & sx, float & sy )
{
	sx = wx;
	sy = wy;
} // world_to_screen





struct RenderContext
{
	RenderStream & rs;
	renderer::VertexStream & vb;
	
	RenderContext( RenderStream & inrs, renderer::VertexStream & invb ) : rs(inrs), vb(invb) {}
}; // RenderContext

class IGraphicObject
{
public:
	virtual ~IGraphicObject() {}
	virtual void render( RenderContext & context ) = 0;
	virtual void get_scale( float & x, float & y ) = 0;
}; // IGraphicObject


struct AABB2
{
	float left;
	float right;
	float top;
	float bottom;
	
	bool overlaps( const AABB2 & other ) const;
}; // AABB2


bool AABB2::overlaps( const AABB2 & other ) const
{
	if ( this->left > other.right )
	{
		return false;
	}
	else if ( this->right < other.left )
	{
		return false;
	}

	if ( this->bottom < other.top )
	{
		return false;
	}
	else if ( this->top > other.bottom )
	{
		return false;
	}
	
	
	return true;
}


class ICollisionObject
{
public:
	virtual ~ICollisionObject() {}
	virtual bool collides_with( ICollisionObject * other ) const = 0;
	virtual void world_position( float & x, float & y ) = 0;
	virtual void step( float dt_sec ) = 0;
	virtual void set_velocity( float x, float y ) = 0;
	virtual void get_aabb( AABB2 & aabb ) const = 0;
	virtual unsigned short get_collision_mask() const = 0;
}; // ICollisionObject




class Sprite : public virtual IGraphicObject, public virtual ICollisionObject
{
public:
	unsigned int material_id;
	renderer::UV texcoords[4];
	Color color;
	
	float world_x;
	float world_y;
	
	float last_world_x;
	float last_world_y;
	
	float r_x;
	float r_y;
	
	short width;
	short height;
	
	
	float velocity_x;
	float velocity_y;

	float scale_x;
	float scale_y;
	
	unsigned short collision_mask;
	
	Sprite()
	{
		world_x = world_y = 0;
		material_id = 0;
		color = Color( 255, 255, 255, 255 );
		memset( texcoords, 0, 4 * sizeof(renderer::UV) );
		
		sprite::calc_tile_uvs( (float*)texcoords, 0, 0, 32, 32, 256, 256 );
		
		velocity_x = velocity_y = 0;
		scale_x = scale_y = 1.0f;
		
		collision_mask = 0;
	}
	
	void select_sprite( int x, int y )
	{
		sprite::calc_tile_uvs( (float*)texcoords, x, y, 32, 32, 256, 256 );
	} // select_sprite
	
	// IGraphicObject
	virtual void render( RenderContext & context )
	{
		float sx, sy;
		world_to_screen( r_x, r_y, sx, sy );
		
		float scx, scy;
		get_scale( scx, scy );
		
		add_sprite_to_stream( context.vb, sx, sy, scx*32, scy*32, color, (float*)texcoords );
	} // render
	
	virtual void get_scale( float & x, float & y )
	{
		x = scale_x;
		y = scale_y;
	} // get_scale
	
	// ICollisionObject
	virtual bool collides_with( ICollisionObject * other ) const
	{
		AABB2 mine;
		AABB2 yours;
		
		// check collision masks
		if ( (this->get_collision_mask() & other->get_collision_mask()) == 0 )
			return false;
		
		this->get_aabb( mine );
		other->get_aabb( yours );

		return mine.overlaps( yours );
	} // collides_with
	
	virtual void world_position( float & x, float & y )
	{
		
	} // world_position
	
	virtual void step( float dt_sec )
	{
		this->last_world_x = this->world_x;
		this->last_world_y = this->world_y;
		this->world_x += dt_sec * velocity_x;
		this->world_y += dt_sec * velocity_y;
		
	} // step
	
	virtual void set_velocity( float x, float y )
	{
		velocity_x = x;
		velocity_y = y;
	} // set_velocity


	void get_aabb( AABB2 & aabb ) const
	{
		float hw = (this->width/2.0f);
		float hh = (this->height/2.0f);
		aabb.left = this->world_x - hw;
		aabb.right = this->world_x + hw;
		aabb.top = this->world_y - hh;
		aabb.bottom = this->world_y + hh;
	} // get_aabb

	unsigned short get_collision_mask() const
	{
		return collision_mask;
	} // get_collision_mask

	void snap_to_world_position( float x, float y )
	{
		this->r_x = this->world_x = this->last_world_x = x;
		this->r_y = this->world_y = this->last_world_y = y;
	} // snap_to_world_position
}; // Sprite



struct Entity
{
	IGraphicObject * graphic_object;
	ICollisionObject * collision_object;
	
	Entity() : graphic_object(0), collision_object(0) {}
}; // Entity

struct MovementCommand
{
	float up, down, left, right;
}; // MovementCommand


ScreenController * screen_controller = 0;


struct LogoScreen : public virtual IScreen
{
	font::Handle font;
	LogoScreen()
	{
		font = font::load_font_from_file( "fonts/nokiafc22.ttf", 8 );
	}

	virtual void on_show( kernel::IApplication * app )
	{
		LOGV( "LogoScreen on show\n" );
	}
	
	virtual void on_hide( kernel::IApplication * app )
	{
		LOGV( "LogoScreen on hide\n" );
	}
	
	virtual void on_draw( kernel::IApplication * app )
	{
		const char text[] = "LogoScreen";
		int center = (kernel::instance()->parameters().render_width / 2);
		int font_width = font::measure_width( font, text );
#if TEST_FONT
		font::draw_string( font, center-(font_width/2), 150, text, Color(255,255,255) );
#endif
	}
	
	virtual void on_update( kernel::IApplication * app ) {}
	
	virtual void on_step( kernel::IApplication * app ) {}
	
	
	virtual const char * name() const
	{
		return "LogoScreen";
	}

	void skip_screen( kernel::IApplication * app )
	{
		screen_controller->pop_screen( app );
	}

	// any event that happens during the logo screen triggers a skip to the next screen
	virtual void on_event( kernel::KeyboardEvent & event, kernel::IApplication * app )
	{
		skip_screen( app );
	}
	
	virtual void on_event( kernel::MouseEvent & event, kernel::IApplication * app )
	{
		// except mouse moved events.
		if ( event.subtype != kernel::MouseMoved )
		{
			skip_screen( app );
		}
	}
	
	virtual void on_event( kernel::TouchEvent & event, kernel::IApplication * app )
	{
		skip_screen( app );
	}
}; // LogoScreen


struct HelpScreen : public virtual IScreen
{
	virtual void on_show( kernel::IApplication * app )
	{
		LOGV( "HelpScreen on show\n" );
	}
	virtual void on_hide( kernel::IApplication * app )
	{
		LOGV( "HelpScreen on hide\n" );
	}
	
	virtual void on_draw( kernel::IApplication * app ) {}
	virtual void on_update( kernel::IApplication * app ) {}
	virtual void on_step( kernel::IApplication * app ) {}
	
	virtual const char * name() const
	{
		return "HelpScreen";
	}
	
	virtual void on_event( kernel::KeyboardEvent & event, kernel::IApplication * app ) {}
	virtual void on_event( kernel::MouseEvent & event, kernel::IApplication * app ) {}
	virtual void on_event( kernel::TouchEvent & event, kernel::IApplication * app ) {}
}; // HelpScreen

struct SpriteVertexType
{
	float x, y, z;
	Color color;
	float u, v;
};

void render_vertexstream( Camera & camera, renderer::VertexStream & vb, RenderStream & rs, unsigned int attributes, assets::Material * material )
{
	long offset = rs.stream.offset_pointer();
	
	assert( material != 0 );
	
	vb.update();
	assets::Shader * shader = assets::find_compatible_shader( attributes + material->requirements );
	rs.add_shader( shader );
	
	rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
	rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
	rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &objectMatrix );
	
	rs.add_material( material, shader );
	rs.add_draw_call( vb.vertexbuffer );
	
	rs.run_commands();
	vb.reset();
	rs.stream.seek( offset, true );
} // render_vertexstream

void add_sprite_to_stream( renderer::VertexStream & vb, int x, int y, int width, int height, const Color & color, float * texcoords )
{
	if ( vb.has_room(4, 6) )
	{
		SpriteVertexType * v = (SpriteVertexType*)vb.request(4);

		v[0].x = x;
		v[0].y = y;
		v[0].z = 0;
		v[0].color = Color(255,255,255);
		
		v[1].x = x;
		v[1].y = y+height;
		v[1].z = 0;
		v[1].color = Color(255,255,255);
		
		v[2].x = x+width;
		v[2].y = y+height;
		v[2].z = 0;
		v[2].color = Color(255,255,255);
		
		v[3].x = x+width;
		v[3].y = y;
		v[3].z = 0;
		v[3].color = Color(255,255,255);
		
		v[0].u = texcoords[0];
		v[0].v = texcoords[1];
		v[1].u = texcoords[2];
		v[1].v = texcoords[3];
		v[2].u = texcoords[4];
		v[2].v = texcoords[5];
		v[3].u = texcoords[6];
		v[3].v = texcoords[7];
		
		
		
//		LOGV( "[%g %g, %g %g, %g %g, %g %g\n", v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, v[3].x, v[3].y );
		
		renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		vb.append_indices( indices, 6 );
	}
}

void render_tiled_map( TiledMap & tiled_map, RenderStream & rs, renderer::VertexStream & vb, unsigned int test_attribs, Camera & camera )
{
	// could potentially have a vertexbuffer per tileset
	// this would allow us to batch tiles that share the same tileset (texture)
	TileSet * lastset = 0;
	
	for( unsigned int layer_num = 0; layer_num < 1/*tiled_map.layer_count*/; ++layer_num )
	{
		for( int h = 0; h < tiled_map.height; ++h )
		{
			for( int w = 0; w < tiled_map.width; ++w )
			{
				unsigned char tile_gid = tiled_map.layers[ layer_num ].layer_data[ h * tiled_map.width + w ];
				if ( tile_gid > 0 )
				{
					Tile * tile = &tiled_map.tilelist.tiles[ tile_gid-1 ];
					if ( tile )
					{
						TileSet * set = &tiled_map.tilesets[ tile->tileset_id ];
						
						if ( (!vb.has_room(4, 6) || (set != lastset)) && lastset != 0 )
						{
							render_vertexstream( camera, vb, rs, test_attribs, lastset->material );							
						}
						
						lastset = set;
						int width = tiled_map.tile_width;
						int height = tiled_map.tile_height;
						int x = w * tiled_map.tile_width;
						int y = h * tiled_map.tile_height;
						
						add_sprite_to_stream( vb, x, y, width, height, Color(255,255,255), tile->quad_uvs );
					} // tile
				} // tile gid > 0
			} // for width
		} // for height
	} // for each layer
	
	if ( vb.last_index > 0 && lastset )
	{
		render_vertexstream( camera, vb, rs, test_attribs, lastset->material );
	}
	
	vb.reset();
}



struct RenameThisData
{
	int world_x;
	int world_y;
	renderer::UV uvs[4];
};


bool is_within_screen( ICollisionObject * object )
{
	float x, y;
	
	object->world_position( x, y );
	
	if ( x < 0 )
	{
		return false;
	}
	else if ( x > kernel::instance()->parameters().render_width )
	{
		return false;
	}

	if ( y < 0 )
	{
		return false;
	}
	else if ( y > kernel::instance()->parameters().render_height )
	{
		return false;
	}


	return true;
}

void constrain_to_screen( Sprite & sprite )
{
	if ( sprite.world_x < 0 )
	{
		sprite.world_x = 0;
	}
	else if ( (sprite.world_x+(sprite.width*sprite.scale_x)) > kernel::instance()->parameters().render_width )
	{
		sprite.world_x = kernel::instance()->parameters().render_width - (sprite.width*sprite.scale_x);
	}
	
	if ( sprite.world_y < 0 )
	{
		sprite.world_y = 0;
	}
	else if ( (sprite.world_y+(sprite.height*sprite.scale_y)) > kernel::instance()->parameters().render_height )
	{
		sprite.world_y = kernel::instance()->parameters().render_height - (sprite.height*sprite.scale_y);
	}
}

void move_sprite_with_command( Sprite & sprite, MovementCommand & command )
{
	const float MOVE_SPEED = 200;
	
	sprite.world_y -= command.up * kernel::instance()->parameters().step_interval_seconds * MOVE_SPEED;
	sprite.world_y += command.down * kernel::instance()->parameters().step_interval_seconds * MOVE_SPEED;
	sprite.world_x -= command.left * kernel::instance()->parameters().step_interval_seconds * MOVE_SPEED;
	sprite.world_x += command.right * kernel::instance()->parameters().step_interval_seconds * MOVE_SPEED;
}


const int MAX_ENTITIES = 1024;

struct GameScreen : public virtual IScreen
{
	font::Handle font;
	Camera camera;
	renderer::VertexStream vb;
	assets::Shader default_shader;
	unsigned int test_attribs;
	TiledMap tiled_map;
	RenderStream rs;
	assets::Material * player_mat;
	
	Sprite * player;
	
	Sprite entities[ MAX_ENTITIES ];
	bool active_entities[ MAX_ENTITIES ];
	
	audio::SoundHandle player_fire, enemy_explode;
	audio::SoundSource player_source;
	short fire_delay;
	short next_fire;
	
	// scrolling layer
	unsigned int background_material_id;
	unsigned short background_num_columns;
	unsigned short background_tile_size;
	unsigned short background_num_rows;
	RenameThisData * background_layers;
	gemini::Recti cliprect;

	EventBasedMap event_based_map;
	unsigned int current_event;
	float current_gametime;
	
	GameScreen()
	{
	
		// need to replace font loading with this ...
//		assets::load_font( "fonts/nokiafc22.ttf", 16 );
		font = font::load_font_from_file( "fonts/nokiafc22.ttf", 16, 72, 72 );
		
		assets::ShaderString name("uv0");
		test_attribs = 0;
		test_attribs |= assets::find_parameter_mask( name );
		
		name = "colors";
		test_attribs |= assets::find_parameter_mask( name );
		
		util::json_load_with_callback( "maps/test.json", tiled_map_loader, &tiled_map, true );

		// load map events
		util::json_load_with_callback( "maps/space.json", map_event_loader, &event_based_map, true );
		current_event = 0;
		current_gametime = 0;
				
		player_mat = assets::load_material("materials/player");
		
		assets::Material * background_material = assets::load_material("materials/background");
		if ( background_material )
		{
			background_material_id = background_material->Id();
		}
		
		cliprect.left = 0;
		cliprect.top = 0;
		cliprect.right = kernel::instance()->parameters().render_width;		
		cliprect.bottom = kernel::instance()->parameters().render_height;


//		cliprect.left = 50;
//		cliprect.top = 50;
//		cliprect.right = 425;
//		cliprect.bottom = 425;

		LOGV( "determining number of tiles needed for viewport: %i x %i\n", kernel::instance()->parameters().render_width, kernel::instance()->parameters().render_height );
		background_tile_size = 64;
		background_num_columns = ceil( cliprect.width() / (float)background_tile_size ) + 1;
		background_num_rows = ceil( cliprect.height() / (float)background_tile_size ) + 1;
		LOGV( "num columns for bg: %i, num rows for bg: %i\n", background_num_columns, background_num_rows );
		
		unsigned int total_background_sprites = background_num_rows * background_num_columns;
		
		background_layers = CREATE_ARRAY(RenameThisData, total_background_sprites);
		memset(background_layers, 0, sizeof(RenameThisData) * total_background_sprites);
		
		RenameThisData * column = background_layers;
		unsigned int y = cliprect.top;
		for( unsigned int r = 0; r < background_num_rows; ++r )
		{
			unsigned int x = cliprect.left;
			for( unsigned int i = 0; i < background_num_columns; ++i )
			{
				sprite::calc_tile_uvs( (float*)column[i].uvs, 0, 0, 64, 64, 64, 64 );
				column[i].world_x = x;
				column[i].world_y = y;
				x += background_tile_size;
			}
			
			column += background_num_columns;
			y += background_tile_size;
		}
		
		// setup the vertex stream: make sure we have enough vertices & indices
		// to accomodate the full background layer
		unsigned int max_vertices = (6 * background_num_columns * background_num_rows);
		unsigned int max_indices = (6 * background_num_columns * background_num_rows);
		vb.reset();
		vb.desc.add( renderer::VD_FLOAT3 );
		vb.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		vb.desc.add( renderer::VD_FLOAT2 );
		vb.create( max_vertices, max_indices, renderer::DRAW_INDEXED_TRIANGLES );
		LOGV( "allocating room for %i max vertices\n", max_vertices );
		
		
		// setup entities
		for( int i = 0; i < MAX_ENTITIES; ++i )
		{
			active_entities[i] = false;
		}
		
		// setup the player sprite (should eventually be loaded in)
		player = &entities[0];
		active_entities[0] = true;
		player->collision_mask = 1;
		
		player->width = 32;
		player->height = 32;

		// set initial position
		player->snap_to_world_position(50, (kernel::instance()->parameters().render_height / 2) - (player->height/2) );
		
		
		// load sounds
		fire_delay = 200;
		next_fire = 0;
		player_fire = audio::create_sound("sounds/blaster1");
		
		enemy_explode = audio::create_sound( "sounds/enemy_explode1");
		

	}
	
	~GameScreen()
	{
		if ( renderer::driver() )
		{
			renderer::driver()->shaderprogram_destroy( this->default_shader );
		}
		
		vb.destroy();
				
		if ( background_layers )
		{
			DESTROY_ARRAY(RenameThisData, background_layers, background_num_columns);
		}
	}
	
	void render_layer( RenameThisData * layer )
	{
		RenameThisData * l = 0;

		for( unsigned short i = 0; i < background_num_columns; ++i )
		{
			l = &layer[ i ];
			add_sprite_to_stream( vb, l->world_x, l->world_y, 64, 64, Color(255,255,255), (float*)l->uvs );
			if ( l->world_x + background_tile_size < cliprect.left )
			{
				l->world_x = l->world_x + background_tile_size * background_num_columns - 1;
			}
			else
			{
				l->world_x = l->world_x - 1;
			}
		}
	} // render_layer

	virtual void on_show( kernel::IApplication * app )
	{
		LOGV( "GameScreen on show\n" );
	}
	virtual void on_hide( kernel::IApplication * app )
	{
		LOGV( "GameScreen on hide\n" );
	}
	
	virtual void on_draw( kernel::IApplication * app )
	{
		kernel::Params & params = kernel::instance()->parameters();
		
		// previously had floating point errors on android when this was only set during startup
		camera.ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -1.0f, 1.0f );
		
		rs.rewind();
		
		rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		rs.add_state( renderer::STATE_BLEND, 1 );
		

		
		
		
#if 0
		// 1 - draw the tile map
		
		//render_tiled_map( tiled_map, rs, vb, test_attribs, camera );
#endif

#if 0 // 2 draw the sprite
		renderer::UV uvs[4];
		
		sprite::calc_tile_uvs( (float*)uvs, 64, 0, 32, 32, 256, 256 );
//		uvs[0].u = 0;
//		uvs[0].v = 0;
//		uvs[1].u = 0;
//		uvs[1].v = 1;
//		uvs[2].u = 1;
//		uvs[2].v = 1;
//		uvs[3].u = 1;
//		uvs[3].v = 0;
		
		add_sprite_to_stream( vb, 0, 0, 32, 32, Color(255,255,255), (float*)uvs );

		
		if ( player_mat )
		{
			render_vertexstream( camera, vb, rs, test_attribs, player_mat );
		}
		else
		{
			LOGW( "error loading player_mat material!\n" );
		}
#endif

		RenderContext context( rs, vb );
		
		// 3 - draw background
		unsigned int num_rows = background_num_rows;
		for( unsigned int i = 0; i < num_rows; ++i )
		{
			render_layer( &background_layers[i * background_num_columns] );
		}
		
		render_vertexstream( camera, vb, rs, test_attribs, assets::load_material("materials/background") );
		vb.reset();
		
		// 4 - draw other entities (enemies/powerups)
		Sprite * ent = 0;
		for( int i = 0; i < MAX_ENTITIES; ++i )
		{
			ent = &entities[i];
			if ( active_entities[i] )
			{
				ent->r_x = lerp( ent->last_world_x, ent->world_x, kernel::instance()->parameters().step_alpha );
				ent->r_y = lerp( ent->last_world_y, ent->world_y, kernel::instance()->parameters().step_alpha );
				
				int left_side = ((ent->r_x + (ent->width * ent->scale_x)) > kernel::instance()->parameters().render_width) && (ent->velocity_x > 0);
				int right_side = ((ent->r_x + (ent->width * ent->scale_x)) < 0) && (ent->velocity_x < 0);
				
				if ( left_side || right_side )
				{
					active_entities[i] = false;
					ent = 0;
				}
				
				if ( active_entities[i] && ent )
				{
					ent->render( context );
				}
			}
		}
	
		// 5 - draw sprite with class?
		// interpolate between two states and get the render position for this sprite
		player->r_x = lerp( player->last_world_x, player->world_x, kernel::instance()->parameters().step_alpha );
		player->r_y = lerp( player->last_world_y, player->world_y, kernel::instance()->parameters().step_alpha );
		
		player->render( context );
		
		if ( player_mat )
		{
			render_vertexstream( camera, vb, rs, test_attribs, player_mat );
		}

		rs.run_commands();
		vb.reset();
//		const char text[] = "---- Game ----";
//		int center = (kernel::instance()->parameters().render_width / 2);
//		int font_width = font::measure_width( font, text );
//		
//		font::draw_string( font, center-(font_width/2), 40, text, Color(0,255,0) );



		font::draw_string( font, 15, 55, xstr_format("dt: %g\n", kernel::instance()->parameters().framedelta_raw), Color(0,255,255));
	}
	
	Sprite * get_unused_entity()
	{
		Sprite * ent = 0;
		
		for( int i = 0; i < MAX_ENTITIES; ++i )
		{
			if ( !active_entities[i] )
			{
				ent = &entities[i];
				active_entities[i] = 1;
				break;
			}
		}
		
		if ( !ent )
		{
			LOGV( "reached max entities\n" );
		}
		
		return ent;
	} // get_unused_entity
	
	bool create_bullet_effect( float x, float y )
	{
		Sprite * ent = get_unused_entity();
		if ( ent )
		{
			ent->snap_to_world_position( x, y );
			ent->set_velocity( BULLET_SPEED, 0 );
			ent->select_sprite(0, 32);
			ent->collision_mask = 2;
			return true;
		}
		
		return false;
	} // create_bullet_effect
	
	
	void add_enemy( float x, float y )
	{
		x *= kernel::instance()->parameters().render_width;
		y *= kernel::instance()->parameters().render_height;
		Sprite * ent = get_unused_entity();
		if ( ent )
		{
			ent->snap_to_world_position( x, y );
			ent->set_velocity( -100, 0 );
			ent->select_sprite(64, 0);
			ent->width = 32;
			ent->height = 32;
			ent->collision_mask = 7;
		}
	} // add_enemy
	
	virtual void on_update( kernel::IApplication * app )
	{
		next_fire -= kernel::instance()->parameters().framedelta_filtered;
		
		if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
		{
			if (next_fire <= 0)
			{
				if ( create_bullet_effect( player->world_x, player->world_y ) )
				{
					next_fire = fire_delay;
					player_source = audio::play( player_fire );
				}
			}
		}
		else if ( input::state()->mouse().is_down( input::MOUSE_RIGHT ) )
		{
			add_enemy( 0.6, 0.2 );
		}
		
		
		current_gametime += (kernel::instance()->parameters().framedelta_filtered*.001);
		
		while ( current_event < event_based_map.total_events )
		{
			MapEvent * ev = &event_based_map.events[ current_event ];
			if ( current_gametime >= ev->time_value )
			{
				LOGV( "run event: %s, at %g seconds\n", ev->name(), current_gametime );
				add_enemy( 1.0, ev->pos );
				++current_event;
				continue;
			}

			break;
		}
	}
	
	float lerp( float a, float b, float t )
	{
		return a + (t * (b-a));
	}
	
	virtual void on_step( kernel::IApplication * app )
	{
#if 0 // don't move the camera in this instance
		kernel::Params & params = kernel::instance()->parameters();
		if ( input::state()->keyboard().is_down( input::KEY_W ) )
		{
			camera.move_forward( params.step_interval_seconds );
		}
		else if ( input::state()->keyboard().is_down( input::KEY_S ) )
		{
			camera.move_backward( params.step_interval_seconds );
		}
		
		if ( input::state()->keyboard().is_down( input::KEY_A ) )
		{
			camera.move_left( params.step_interval_seconds );
		}
		else if ( input::state()->keyboard().is_down( input::KEY_D ) )
		{
			camera.move_right( params.step_interval_seconds );
		}
#endif

		// instead, move the sprite (we do need some interpolation here otherwise the stuttering is visible)
		player->last_world_x = player->world_x;
		player->last_world_y = player->world_y;
		
		for( int i = 0; i < MAX_ENTITIES; ++i )
		{
			ICollisionObject * ent = &entities[i];
			if ( active_entities[i] )
			{
				ent->step( kernel::instance()->parameters().step_interval_seconds );
				
				if ( is_within_screen(ent) )
				{
					for( int j = 0; j < MAX_ENTITIES; ++j )
					{
						if ( i != j && active_entities[j] && is_within_screen(&entities[j]) )
						{
							if ( ent->collides_with(&entities[j]) )
							{
								ent->set_velocity(0, 0);						
								entities[j].set_velocity(0, 0);
								active_entities[i] = false;
								active_entities[j] = false;
								
								audio::play( enemy_explode );
							}
						}
					}
				}
			}
		}
		
		

				
		//
		MovementCommand command;
		command.up = input::state()->keyboard().is_down( input::KEY_W );
		command.down = input::state()->keyboard().is_down( input::KEY_S );
		command.left = input::state()->keyboard().is_down( input::KEY_A );
		command.right = input::state()->keyboard().is_down( input::KEY_D );
		
		// move player
		move_sprite_with_command( *player, command );
		
		// constrain the player within the bounds of the window
		constrain_to_screen( *player );
	}
	
	virtual const char * name() const
	{
		return "GameScreen";
	}
	
	virtual void on_event( kernel::KeyboardEvent & event, kernel::IApplication * app )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
		}
	}
	virtual void on_event( kernel::MouseEvent & event, kernel::IApplication * app ) {}
	virtual void on_event( kernel::TouchEvent & event, kernel::IApplication * app ) {}
}; // GameScreen


using namespace kernel;

class TestUniversal : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>,
	public IEventListener<TouchEvent>
{
	RenderStream rs;

	int tdx;
	int tdy;
public:
	DECLARE_APPLICATION( TestUniversal );

	TestUniversal()
	{
		screen_controller = CREATE(ScreenController);
	}
	
	
	~TestUniversal()
	{
		DESTROY(ScreenController, screen_controller);
	}
	
	virtual void event( kernel::TouchEvent & event )
	{
		IScreen * screen = screen_controller->active_screen();
		if ( screen )
		{
			screen->on_event( event, this );
		}
#if 0
		if ( event.subtype == kernel::TouchBegin )
		{
//			fprintf( stdout, "Touch Event Began at %i, %i\n", event.x, event.y );
			tdx = event.x;
			tdy = event.y;
		}
		else if ( event.subtype == kernel::TouchMoved )
		{
//			fprintf( stdout, "Touch Event Moved at %i, %i\n", event.x, event.y );
			this->camera.move_view( (event.x - tdx), (event.y - tdy) );
			tdx = event.x;
			tdy = event.y;
		}
		else if ( event.subtype == kernel::TouchEnd )
		{
//			fprintf( stdout, "Touch Event Ended at %i, %i\n", event.x, event.y );
		}
#endif
	}
	
	virtual void event( KeyboardEvent & event )
	{
		IScreen * screen = screen_controller->active_screen();
		if ( screen )
		{
			screen->on_event( event, this );
		}
		
#if 0
        if ( event.is_down )
        {
            fprintf( stdout, "key %i pressed\n", event.key );
			 
			if ( event.key == input::KEY_Q )
			{
//				audio::stop( source );
			}
        }
        else
        {
            fprintf( stdout, "key %i released\n", event.key );
        }
#endif
	}

	virtual void event( MouseEvent & event )
	{
		IScreen * screen = screen_controller->active_screen();
		if ( screen )
		{
			screen->on_event( event, this );
		}
#if 0
        switch( event.subtype )
        {
            case kernel::MouseMoved:
			{
				if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
				{
					int lastx, lasty;
					input::state()->mouse().last_mouse_position( lastx, lasty );
					
					camera.move_view( event.mx-lastx, event.my-lasty );
				}
                break;
			}
            case kernel::MouseButton:
                if ( event.is_down )
                {
                    fprintf( stdout, "mouse button %i is pressed\n", event.button );
                }
                else
                {
                    fprintf( stdout, "mouse button %i is released\n", event.button );
                }
                break;
                
            case kernel::MouseWheelMoved:
                if ( event.wheel_direction > 0 )
                {
                    fprintf( stdout, "mouse wheel toward screen\n" );
                }
                else
                {
                    fprintf( stdout, "mouse wheel away from screen\n" );
                }
                break;
            default:
                fprintf( stdout, "mouse event received!\n" );
                break;
        }
#endif

	}

	virtual void event( SystemEvent & event )
	{
		switch( event.subtype )
		{
			case kernel::WindowGainFocus:
				fprintf( stdout, "window gained focus\n" );
				break;
				
			case kernel::WindowLostFocus:
				fprintf( stdout, "window lost focus\n" );
				break;
				
			case kernel::WindowResized:
				fprintf( stdout, "resize event: %i x %i\n", event.window_width, event.window_height );
				break;
				
			default: break;
		}

	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 720;
		params.window_height = 480;
		params.window_title = "TestUniversal";
//		HSQUIRRELVM vm = sq_open(1024);
//		sq_close( vm );
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		LOGV( "Window dimensions: %i x %i, Render Viewport: %i x %i\n", params.window_width, params.window_height, params.render_width, params.render_height );
		LOGV( "IndexType is %i bytes.\n", sizeof(renderer::IndexType) );
//		sound = audio::create_sound( "sounds/powerup" );
//		source = audio::play( sound );
		
		LogoScreen * logo = CREATE(LogoScreen);
		HelpScreen * help = CREATE(HelpScreen);
		GameScreen * game = CREATE(GameScreen);

		// make the controller aware of these screens
		screen_controller->add_screen( logo );
		screen_controller->add_screen( help );
		screen_controller->add_screen( game );

		// setup the stack
		screen_controller->push_screen( "GameScreen", this );
//		screen_controller->push_screen( "LogoScreen", this );

		return kernel::Application_Success;
	}

	
	virtual void step( kernel::Params & params )
	{
		if (screen_controller->active_screen())
		{
			screen_controller->active_screen()->on_step( this );
		}
	}

	virtual void tick( kernel::Params & params )
	{
		if ( screen_controller->active_screen() )
		{
			screen_controller->active_screen()->on_update( this );
		}
	
		rs.rewind();
		
		// setup global rendering state
		rs.add_clearcolor( 0.15, 0.10, 0.25, 1.0f );
		rs.add_clear( 0x00004000 | 0x00000100 );
		rs.add_viewport( 0, 0, (int)params.render_width, (int)params.render_height );

		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		rs.run_commands();
		rs.rewind();
		
		if ( screen_controller->active_screen() )
		{
			screen_controller->active_screen()->on_draw( this );
		}

//		font::draw_string( test_font, 50, 50, xstr_format("deltatime: %gms", params.framedelta_filtered), Color(255,128,0,255) );
//		font::draw_string( test_font, 50, 75, "Ја могу да једем стакло", Color(255, 255, 255, 255) );
//		font::draw_string( test_font, 50, 100, "私はガラスを食べられます。それは私を傷つけません。", Color(0, 128, 255, 255) );
	}

	virtual void shutdown( kernel::Params & params )
	{
	}
};

IMPLEMENT_APPLICATION( TestUniversal );



// experimental



// mesh rendering
#if 0
Camera camera;
glm::vec3 light_position = glm::vec3( 0, 2, 0 );
//assets::Geometry geo;
assets::Mesh * mesh;

void startup()
{
	alpha = 0;
	alpha_delta = 1;

	// test mesh loading
	mesh = assets::load_mesh( "models/plasma3" );
	if ( mesh )
	{
		mesh->prepare_geometry();
	}
	else
	{
		LOGW( "unable to load mesh.\n" );
	}
	
#if 0
	geo.vertex_count = 4;
	geo.index_count = 6;
	geo.vertices = CREATE_ARRAY( glm::vec3, 4 );
	geo.indices = CREATE_ARRAY( renderer::IndexType, 6 );
	geo.colors = CREATE_ARRAY( Color, 4 );
	geo.uvs = CREATE_ARRAY( renderer::UV, 4 );
	
	geo.draw_type = renderer::DRAW_INDEXED_TRIANGLES;
	geo.material_id = assets::load_material("materials/default")->Id();
	
	glm::vec3 * vertices = geo.vertices;
	Color * colors = geo.colors;
	renderer::UV * uvs = geo.uvs;
	vertices[0] = glm::vec3(0,0,0);
	colors[0].set( 255, 255, 255 );
	uvs[0].u = 0;
	uvs[0].v = 0;
	
	vertices[1] = glm::vec3(0, TEST_SIZE, 0);
	colors[1].set( 255, 255, 255 );
	uvs[1].u = 0;
	uvs[1].v = 1;
	
	vertices[2] = glm::vec3(TEST_SIZE, TEST_SIZE, 0);
	colors[2].set( 255, 255, 255 );
	uvs[2].u = 1;
	uvs[2].v = 1;
	
	vertices[3] = glm::vec3(TEST_SIZE, 0, 0);
	colors[3].set( 255, 255, 255 );
	uvs[3].u = 1;
	uvs[3].v = 0;
	
	renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
	memcpy( geo.indices, indices, sizeof(renderer::IndexType) * geo.index_count );
	
	
	geo.render_setup();
#endif
	
}

void step()
{
	alpha += ((float)alpha_delta) * 0.025f;
	
	if ( alpha >= 1.0 || alpha <= 0 )
	{
		if ( alpha >= 1.0 )
		{
			alpha = 1.0f;
		}
		else
		{
			alpha = 0;
		}
		alpha_delta = -alpha_delta;
		
	}
	
	light_position.x = cosf( alpha ) * 4;
	light_position.z = sinf( alpha ) * 4;
}


void tick()
{
	renderer::GeneralParameters gp;
	assets::ShaderString lightposition = "lightposition";
	gp.global_params = 0; //assets::find_parameter_mask( lightposition );
	gp.camera_position = &camera.pos;
	gp.modelview_matrix = &camera.matCam;
	gp.projection_project = &camera.matProj;
	gp.object_matrix = &objectMatrix;
	
	// rotate the model
	//		objectMatrix = glm::rotate( objectMatrix, 0.5f, glm::vec3( 0, 1, 0) );
	camera.perspective( 60, params.render_width, params.render_height, 0.1f, 512.0f );
	
	if ( mesh )
	{
		for( unsigned int geo_id = 0; geo_id < mesh->total_geometry; ++geo_id )
		{
			assets::Geometry * g = &mesh->geometry[ geo_id ];
			render_utilities::stream_geometry( rs, g, gp );
		}
	}
}
#endif




// menu stuff
#if 0



#if 0
setup_menu();

foreach_child( _menu.current_menu(), print_option );
_menu.navigate_to_child( 0 );
foreach_child( _menu.current_menu(), print_option );


_menu.navigate_back();
foreach_child( _menu.current_menu(), print_option );

_menu.clear_items();
#endif



void foreach_child( MenuItem * root, foreach_menu_callback callback )
{
	MenuItemVector::iterator it, end;
	it = root->children.begin();
	end = root->children.end();
	
	for( ; it != end; ++it )
	{
		MenuItem * option = (*it);
		callback( option );
	}
}

MenuNavigator _menu;

void setup_menu()
{
	MenuItem * root = _menu.root_menu();
	root->name = "Game";
	
	MenuItem * newgame = root->add_child( "New Game" );
	newgame->add_child( "Easy" );
	newgame->add_child( "Medium" );
	newgame->add_child( "Hard" );
	
	root->add_child( "Load Game" );
	root->add_child( "Options" );
	root->add_child( "Quit" );
}

void print_option( MenuItem * child )
{
	LOGV( "[ %s ]\n", child->name );
}


void print_options( MenuItem * root )
{
	LOGV( "[ %s ]\n", root->name );
	LOGV( "options:\n" );
	
	MenuItemVector::iterator it, end;
	it = root->children.begin();
	end = root->children.end();
	
	for( ; it != end; ++it )
	{
		MenuItem * option = (*it);
		LOGV( "-> %s\n", option->name );
	}
}
#endif
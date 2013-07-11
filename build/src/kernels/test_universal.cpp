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
#include "keyframechannel.hpp"
#include "debugdraw.hpp"
#include "util.hpp"

#define TEST_2D 0
#define TEST_FONT 1

glm::mat4 objectMatrix;

const float BULLET_SPEED = 500;
const float TEST_SIZE = 256;
const int STARTING_ENERGY = 10;

enum {
	GAME_PLAY = 1,
	GAME_WIN,
	GAME_FAIL
};

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


class IComponent
{
public:
	virtual ~IComponent() {}
};

class IGraphicObject
{
public:
	virtual ~IGraphicObject() {}
	virtual void render( RenderContext & context ) = 0;
	virtual void get_scale( float & x, float & y ) = 0;
	virtual Color get_color() = 0;
	virtual void update( float dt_sec ) = 0;
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
	virtual void get_rotation( float & radians ) const = 0;
}; // ICollisionObject


util::ConfigLoadStatus load_sprite_from_file( const Json::Value & root, void * data );

class Sprite : public virtual IGraphicObject, public virtual ICollisionObject
{
public:
	struct SpriteFrame
	{
		renderer::UV texcoords[4];
	};
	
	struct AnimationSequence
	{
		std::string name;
		unsigned short frame_start;
		unsigned short total_frames;
		SpriteFrame * frames;
		
		
		AnimationSequence();
		~AnimationSequence();
		
		void create_frames( unsigned int material_id, unsigned int num_frames, unsigned int sprite_width, unsigned int sprite_height );
		void purge_frames();
		float * uvs_for_frame( unsigned short frame_id );
		bool is_valid_frame(unsigned short frame_id);
	}; // AnimationSequence

	AnimationSequence * animations;		// animation frames
	unsigned short current_animation;	// currently active animation
	unsigned short current_frame;		// current frame of the animation
	unsigned short total_animations;	// total animations
	float animation_time;				// current time of the animation
	float frame_delay;					// delay in msec between each frame

	// legacy
	SpriteFrame frame;
	
	unsigned int material_id;
	Color color;
	
	float world_x;
	float world_y;
	
	float last_world_x;
	float last_world_y;
	
	float r_x;
	float r_y;
	
	unsigned short width;
	unsigned short height;
	
	unsigned short collision_size;
	
	float velocity_x;
	float velocity_y;

	float scale_x;
	float scale_y;
	
	unsigned short collision_mask;
	
	short hotspot_x;
	short hotspot_y;
	
	float rotation;
	
	void reset_components()
	{
		this->color = Color(255, 255, 255);
		this->scale_x = 1.0f;
		this->scale_y = 1.0f;
	} // reset_components
	
	Sprite()
	{
		animations = 0;
		current_frame = 0;
		current_animation = 0;
		animation_time = 0;
		total_animations = 0;
		
		world_x = world_y = 0;
		material_id = 0;
		color = Color( 255, 255, 255, 255 );
		memset( frame.texcoords, 0, 4 * sizeof(renderer::UV) );

		velocity_x = velocity_y = 0;
		scale_x = scale_y = 1.0f;
		
		width = 32;
		height = 32;
		
		hotspot_x = 0;
		hotspot_y = 0;
		
		collision_mask = 0;
		rotation = 0;
		collision_size = 0;
		
		frame_delay = 1.0f; // second
	} // Sprite
	
	~Sprite()
	{
		purge_animations();
	} // ~Sprite
	
	void load_sprite( const char * path )
	{
		util::json_load_with_callback( path, load_sprite_from_file, this, true );
	} // load_sprite
		
	void create_animations( unsigned short num_animations )
	{
		if ( animations && total_animations > 0 )
		{
			purge_animations();
		}
		
		animations = CREATE_ARRAY(AnimationSequence, num_animations );
		total_animations = num_animations;
	} // create_animations
	
	void purge_animations()
	{
		DESTROY_ARRAY( AnimationSequence, animations, total_animations );
	} // purge_animations
	
	void play_animation( const std::string & name )
	{
		current_frame = 0;
		animation_time = 0;
		
		for( unsigned short i = 0; i < total_animations; ++i )
		{
			AnimationSequence * anim = &animations[i];
			if ( name == anim->name )
			{
				current_animation = i;
				return;
			}
		}
		
		LOGV( "unable to find animation: %s\n", name.c_str() );
	} // play_animation
	
	AnimationSequence * get_animation_by_index( unsigned short animation_index )
	{
		if ( animation_index >= 0 && animation_index < this->total_animations )
			return &animations[ animation_index ];
		
		return 0;
	} // get_animation_by_index
	
	// IGraphicObject
	virtual void render( RenderContext & context )
	{
		float sx, sy;
		world_to_screen( r_x, r_y, sx, sy );
		
		float scx, scy;
		get_scale( scx, scy );
		
		
		AnimationSequence * current_sequence = get_animation_by_index( current_animation );
		if ( current_sequence && current_sequence->is_valid_frame(current_frame) )
		{
			add_sprite_to_stream( context.vb, sx, sy, scx*this->width, scy*this->height, this->get_color(), current_sequence->uvs_for_frame( current_frame ) );
		}
		
		
		
		debugdraw::point( glm::vec3(r_x, r_y, 0), Color(255,0,0), scx*(this->collision_size/2.0), 0 );
	} // render
	
	virtual void get_scale( float & x, float & y )
	{
		x = scale_x;
		y = scale_y;
	} // get_scale
	
	virtual Color get_color()
	{
		return color;
	} // get_color
	
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
		x = this->world_x;
		y = this->world_y;
	} // world_position
	
	virtual void step( float dt_sec )
	{
		this->last_world_x = this->world_x;
		this->last_world_y = this->world_y;
		this->world_x += dt_sec * velocity_x;
		this->world_y += dt_sec * velocity_y;
	} // step
	
	
	virtual void update( float dt_sec )
	{
		this->animation_time -= dt_sec;
		
		if ( this->animation_time <= 0 )
		{
			++current_frame;
			this->animation_time = this->frame_delay;
			
			AnimationSequence * sequence = this->get_animation_by_index(current_animation);
			if ( sequence )
			{
				if ( current_frame >= sequence->total_frames )
				{
					// TODO: callback when this sequence finished?
					current_frame = 0;
				}
			}
		}
	} // update
	
	virtual void set_velocity( float x, float y )
	{
		velocity_x = x;
		velocity_y = y;
	} // set_velocity


	void get_aabb( AABB2 & aabb ) const
	{
		float hw = (this->collision_size/2.0f);
		float hh = (this->collision_size/2.0f);
		aabb.left = this->world_x - hw;
		aabb.right = this->world_x + hw;
		aabb.top = this->world_y - hh;
		aabb.bottom = this->world_y + hh;
	} // get_aabb

	unsigned short get_collision_mask() const
	{
		return collision_mask;
	} // get_collision_mask
	
	void get_rotation( float & radians ) const
	{
		radians = rotation;
	} // get_rotation

	void snap_to_world_position( float x, float y )
	{
		this->r_x = this->world_x = this->last_world_x = x;
		this->r_y = this->world_y = this->last_world_y = y;
	} // snap_to_world_position
}; // Sprite


Sprite::AnimationSequence::AnimationSequence()
{
	this->frame_start = 0;
	this->frames = 0;
	this->total_frames = 0;
}

Sprite::AnimationSequence::~AnimationSequence()
{
	purge_frames();
}

void frame_to_pixels( unsigned short frame, assets::Texture * texture, unsigned int sprite_width, unsigned int sprite_height, unsigned int & x, unsigned int & y )
{
	unsigned short cols = (texture->width / sprite_width);
	unsigned short rows = (texture->height / sprite_height);
	
	x = (frame % cols) * sprite_width;
	y = (frame / rows) * sprite_height;
} // frame_to_pixels

void Sprite::AnimationSequence::create_frames(unsigned int material_id, unsigned int num_frames, unsigned int sprite_width, unsigned int sprite_height)
{
	// ...

	assets::Material * material = assets::material_by_id(material_id);
	if ( !material )
	{
		LOGE( "Unable to locate material with id: %i. Cannot load frames!\n", material_id );
		return;
	}
	
	assets::Material::Parameter * parameter = material->parameter_by_name("diffusemap");
	if ( !parameter )
	{
		LOGE( "Unable to find parameter by name: diffusemap\n" );
		return;
	}
	
	assets::Texture * texture = assets::texture_by_id( parameter->intValue );
	if ( !texture )
	{
		LOGE( "Unable to find texture for id: %i\n", parameter->intValue );
		return;
	}
	
	if ( this->frames && total_frames > 0 )
	{
		purge_frames();
	}

	total_frames = num_frames;	
	this->frames = CREATE_ARRAY(SpriteFrame, total_frames);


	unsigned int x = 0;
	unsigned int y = 0;
	for( unsigned int frame = 0; frame < total_frames; ++frame )
	{
		SpriteFrame * sf = &frames[ frame ];
		frame_to_pixels( frame+frame_start, texture, sprite_width, sprite_height, x, y );
		sprite::calc_tile_uvs( (float*)sf->texcoords, x, y, sprite_width, sprite_height, texture->width, texture->height );
	}

} // load_frames

void Sprite::AnimationSequence::purge_frames()
{
	DESTROY_ARRAY(SpriteFrame, frames, total_frames);
	total_frames = 0;
} // purge_frames

float * Sprite::AnimationSequence::uvs_for_frame(unsigned short frame_id)
{
	if (is_valid_frame(frame_id))
	{
		return (float*)&frames[ frame_id ].texcoords;
	}
	
	return 0;
} // uvs_for_frame

bool Sprite::AnimationSequence::is_valid_frame(unsigned short frame_id)
{
	return (frame_id >= 0 && frame_id < total_frames);
} // is_valid_frame


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


void virtual_screen_to_pixels( float & tx, float & ty )
{
	const int VIRTUAL_WIDTH = 800;
	const int VIRTUAL_HEIGHT = 600;
	
	kernel::Params & params = kernel::instance()->parameters();
	
	
	float mx = (params.render_width / (float)VIRTUAL_WIDTH);
	float my = (params.render_height / (float)VIRTUAL_HEIGHT);
	
	tx = mx * (VIRTUAL_WIDTH * tx);
	ty = my * (VIRTUAL_HEIGHT * ty);
}


util::ConfigLoadStatus load_sprite_from_file( const Json::Value & root, void * data )
{
	Sprite * sprite = (Sprite*)data;
	if (!sprite)
	{
		return util::ConfigLoad_Failure;
	}
	
	// check material path; load material and cache the id in the sprite
	Json::Value material_path = root["material"];
	if ( material_path.isNull() )
	{
		LOGE( "material is required\n" );
		return util::ConfigLoad_Failure;
	}
	assets::Material * material = assets::load_material( material_path.asString().c_str() );
	if ( material )
	{
		sprite->material_id = material->Id();
	}
	
	// check width and height; set these in the sprite
	Json::Value width_pixels = root["width"];
	Json::Value height_pixels = root["height"];
	if ( width_pixels.isNull() || height_pixels.isNull() )
	{
		LOGE( "width and height are required\n" );
		return util::ConfigLoad_Failure;
	}
	sprite->width = width_pixels.asUInt();
	sprite->height = height_pixels.asUInt();
	
	// load collision size
	Json::Value collision_width = root["collision_size"];
	if ( collision_width.isNull() )
	{
		LOGE( "collision_size is required\n" );
		return util::ConfigLoad_Failure;
	}
	sprite->collision_size = collision_width.asUInt();
	
	// frame delay
	Json::Value frame_delay = root["frame_delay"];
	if ( !frame_delay.isNull() )
	{
		sprite->frame_delay = frame_delay.asFloat();
	}
	
	// load sprite scale
	Json::Value scale_x = root["scale_x"];
	Json::Value scale_y = root["scale_y"];
	if ( !scale_x.isNull() )
	{
		sprite->scale_x = scale_x.asFloat();
	}
	
	if ( !scale_y.isNull() )
	{
		sprite->scale_y = scale_y.asFloat();
	}
	
	// check for and load all animations
	Json::Value animation_list = root["animations"];
	if ( animation_list.isNull() )
	{
		LOGE( "TODO: handle no animations with a static frame...\n" );
		return util::ConfigLoad_Failure;
	}
	
	sprite->create_animations( animation_list.size() );
	unsigned short animation_index = 0;
	
	Json::ValueIterator iter = animation_list.begin();
	for( ; iter != animation_list.end(); ++iter )
	{
		Json::Value animation = (*iter);
		Json::Value animation_name = animation["name"];
		Json::Value frame_start = animation["frame_start"];
		Json::Value num_frames = animation["num_frames"];
		if ( animation_name.isNull() )
		{
			LOGE( "'name' is required for animation!\n" );
			continue;
		}
		
		if ( frame_start.isNull() )
		{
			LOGE( "'frame_start' is required for animation!\n" );
			continue;
		}
		
		if ( num_frames.isNull() )
		{
			LOGE( "'num_frames' is required for animation!\n" );
		}
		
		Sprite::AnimationSequence * sequence = sprite->get_animation_by_index( animation_index++ );
		sequence->name = animation_name.asString();
		sequence->frame_start = frame_start.asInt();
		sequence->total_frames = num_frames.asInt();
		
		// this can be deferred when I merge sprite sheets
		sequence->create_frames( sprite->material_id, sequence->total_frames, sprite->width, sprite->height );
//		LOGV( "animation name: %s\n", animation_name.asString().c_str() );
//		LOGV( "start: %i, num_frames: %i\n", start.asInt(), num_frames.asInt() );
	}
	
	
	return util::ConfigLoad_Success;
} // load_sprite_from_file


struct LogoScreen : public virtual IScreen
{
	font::Handle font;
	LogoScreen()
	{
		font = font::load_font_from_file( "fonts/nokiafc22.ttf", 64 );
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
	long offset;
	rs.save_offset( offset );
	
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
	rs.load_offset( offset );
} // render_vertexstream

void add_sprite_to_stream( renderer::VertexStream & vb, int x, int y, int width, int height, const Color & color, float * texcoords )
{
	if ( vb.has_room(4, 6) )
	{
		SpriteVertexType * v = (SpriteVertexType*)vb.request(4);
		float hw = width/2.0f;
		float hh = height/2.0f;

		// x and y are assumed to be the center of the sprite
		// upper left corner; moving clockwise
		v[0].x = x - hw;
		v[0].y = y - hh;
		v[0].z = 0;
		v[0].color = color;
		
		v[1].x = x - hw;
		v[1].y = y + hh;
		v[1].z = 0;
		v[1].color = color;
		
		v[2].x = x + hw;
		v[2].y = y + hh;
		v[2].z = 0;
		v[2].color = color;
		
		v[3].x = x + hw;
		v[3].y = y - hh;
		v[3].z = 0;
		v[3].color = color;
		
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

#if 0
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
#endif



struct RenameThisData
{
	int world_x;
	int world_y;
	renderer::UV uvs[4];
};


bool is_within_screen( ICollisionObject * object )
{
	float x = 0, y = 0;
	
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
	float wx, wy;
	sprite.world_position( wx, wy );
	
	float hw = sprite.width/2.0f;
	float hh = sprite.height/2.0f;
	
	float scaled_half_width = (hw * sprite.scale_x);
	float scaled_half_height = (hh * sprite.scale_y);

	if ( wx < scaled_half_width )
	{
		wx = scaled_half_width;
	}
	else if ( (wx+scaled_half_width) > kernel::instance()->parameters().render_width )
	{
		wx = kernel::instance()->parameters().render_width - scaled_half_width;
	}
	
	if ( wy < scaled_half_height )
	{
		wy = scaled_half_height;
	}
	else if ( (wy+scaled_half_height) > kernel::instance()->parameters().render_height )
	{
		wy = kernel::instance()->parameters().render_height - scaled_half_height;
	}
	
	sprite.snap_to_world_position( wx, wy );
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
	font::Handle round_title;
	Camera camera;
	renderer::VertexStream vb;
	assets::Shader default_shader;
	unsigned int test_attribs;
//	TiledMap tiled_map;
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
	
	unsigned int score;
	unsigned int energy;
	unsigned char game_state;
	
	GameScreen()
	{
		energy = STARTING_ENERGY;
		score = 0;
		game_state = GAME_PLAY;
		// need to replace font loading with this ...
//		assets::load_font( "fonts/nokiafc22.ttf", 16 );
		font = font::load_font_from_file( "fonts/nokiafc22.ttf", 16, 72, 72 );
		round_title = font::load_font_from_file( "fonts/nokiafc22.ttf", 32 );
		
		assets::ShaderString name("uv0");
		test_attribs = 0;
		test_attribs |= assets::find_parameter_mask( name );
		
		name = "colors";
		test_attribs |= assets::find_parameter_mask( name );
		
//		util::json_load_with_callback( "maps/test.json", tiled_map_loader, &tiled_map, true );

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

		player->width = 64;
		player->height = 64;
		player->hotspot_x = 0;
		player->hotspot_y = 8;
		player->collision_size = 32;
		player->scale_x = 1.0f;
		player->scale_y = 1.0f;

		// set initial position
		player->snap_to_world_position(50, (kernel::instance()->parameters().render_height / 2) - (player->height/2) );
		
		player->load_sprite("sprites/player.conf");

		// load sounds
		fire_delay = 200;
		next_fire = 0;
		player_fire = audio::create_sound("sounds/blaster1");
		
		enemy_explode = audio::create_sound( "sounds/enemy_explode1");

		this->default_shader.object = 0;
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
				
				if ( !is_within_screen(ent) )
				{
					active_entities[i] = false;
					ent = 0;
				}
				
				if ( active_entities[i] && ent && ent != player )
				{
					ent->update( params.framedelta_filtered * .001f );
					ent->render( context );
				}
			}
		}
	
		// 5 - draw player after other entities
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


//		debugdraw::text( 15, 55, "hello", Color(255,255,128) );
		

//		debugdraw::line( glm::vec3(0, 0, 0), glm::vec3(player->r_x, player->r_y, 0), Color(255, 128, 0));
		

		glm::mat4 modelview;
		glm::mat4 proj = glm::ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -0.1f, 128.0f );
		debugdraw::render( modelview, proj, params.render_width, params.render_height );
		
		
		float tx = .02, ty = 0.05;
		
		virtual_screen_to_pixels( tx, ty );
		
		font::draw_string( font, tx, ty, xstr_format("Energy: %i", this->energy), Color(255,255,255));
		
		
		tx = 0.8;
		ty = 0.05;
		virtual_screen_to_pixels( tx, ty );
		font::draw_string( font, tx, ty, xstr_format("Score: %04i", this->score), Color(255,255,255));
		
		if ( game_state == GAME_FAIL )
		{
			float tx, ty;
			tx = 0.35;
			ty = 0.1;
			virtual_screen_to_pixels(tx, ty);
			font::draw_string( round_title, tx, ty, "Game Fail", Color(255,0,0) );
		}
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
		else
		{
			ent->reset_components();
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
			ent->load_sprite("sprites/bullet.conf");
			ent->collision_mask = 2;
			ent->play_animation("idle");
			return true;
		}
		
		return false;
	} // create_bullet_effect
	
	
	void add_enemy( float x, float y, unsigned int id )
	{
		x *= kernel::instance()->parameters().render_width;
		y *= kernel::instance()->parameters().render_height;
		Sprite * ent = get_unused_entity();
		if ( ent )
		{
			ent->snap_to_world_position( x, y );
			float velocity = 100.0f;
			ent->set_velocity( -velocity, 0 );

			ent->load_sprite( "sprites/enemy.conf" );
			if ( id == 1 )
			{
				ent->color = Color(255, 0, 0);
			}
			ent->collision_mask = 7;
			ent->collision_size = 32;
		}
	} // add_enemy
	
	virtual void on_update( kernel::IApplication * app )
	{
		if ( game_state == GAME_PLAY )
		{
			next_fire -= kernel::instance()->parameters().framedelta_filtered;
			
			if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
			{
				if (next_fire <= 0)
				{
					if ( create_bullet_effect( player->r_x+player->hotspot_x, player->r_y+player->hotspot_y ) )
					{
						next_fire = fire_delay;
	//					player_source = audio::play( player_fire );
					}
				}
			}
#if 0
			else if ( input::state()->mouse().is_down( input::MOUSE_RIGHT ) )
			{
				int x, y;
				input::state()->mouse().mouse_position(x, y);
				create_bullet_effect( x, y );
			}
#endif
			
			current_gametime += (kernel::instance()->parameters().framedelta_filtered*.001);
	#if 1
			while ( current_event < event_based_map.total_events )
			{
				MapEvent * ev = &event_based_map.events[ current_event ];
				if ( current_gametime >= ev->time_value )
				{
	//				LOGV( "run event: %s, at %g seconds\n", ev->name(), current_gametime );
					add_enemy( 1.0, ev->pos, ev->id );
					++current_event;
					continue;
				}

				break;
			}
	#endif
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
		if ( game_state == GAME_PLAY )
		{
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
									if ( player == ent || player == &entities[j] )
									{
										energy -= 1;
										if ( energy <= 0 )
										{
											energy = 0;
											game_state = GAME_FAIL;
										}
										
										if ( player == ent )
										{
											active_entities[j] = false;
										}
										else
										{
											active_entities[i] = false;
										}
									}
									else
									{
										ent->set_velocity(0, 0);
										entities[j].set_velocity(0, 0);
										active_entities[i] = false;
										active_entities[j] = false;
										score += 1;
										audio::play( enemy_explode );
									}
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

		debugdraw::startup( 1024 );

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
	
		debugdraw::update( params.framedelta_filtered );
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
		debugdraw::shutdown();
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
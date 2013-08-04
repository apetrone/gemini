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
#include <slim/xlog.h>
#include "gamescreen.hpp"
#include "render_utilities.hpp"
#include "componentmanager.hpp"
#include "debugdraw.hpp"

#include "components.hpp"


RenderControl render_control;
// game stuff
unsigned int score;
unsigned int energy;
unsigned char game_state;

// -------------------------------------------------------------
#include <vector>


typedef int EntityID;
struct EntityManager
{
	typedef std::vector<EntityID> EntityList;
	EntityID base_entity_id;
	EntityList active, inactive;

	EntityManager();
	EntityID create();
	void remove( EntityID eid );
	
	void _print_lists();
}; // EntityManager


EntityManager::EntityManager() : base_entity_id(0) {}

EntityID EntityManager::create()
{
	EntityID id = -1;
	
	if ( inactive.empty() )
	{
		id = base_entity_id++;
		active.push_back(id);
	}
	else
	{
		id = inactive.back();
		inactive.pop_back();
		active.push_back(id);
	}
	
	return id;
} // create

void EntityManager::remove( EntityID eid )
{
	// find eid in active; remove it.
	// insert it in the inactive list
	EntityList::iterator it = active.begin();
	for( ; it != active.end(); ++it )
	{
		if ( (*it) == eid )
		{
			active.erase(it);
			inactive.push_back(eid);
			break;
		}
	}
} // remove


void EntityManager::_print_lists()
{
	unsigned int index = 0;
	
	LOGV( "active list:\n" );
	EntityList::iterator it = active.begin();
	for( ; it != active.end(); ++it )
	{
		LOGV("%i) %i\n", index, (*it) );
	}
	
	index = 0;
	LOGV( "inactive list:\n" );
	it = inactive.begin();
	for( ; it != inactive.end(); ++it )
	{
		LOGV("%i) %i\n", index, (*it) );
	}
} // _print_lists

// -------------------------------------------------------------
using namespace render_utilities;



void add_sprite_to_layer( unsigned short layer, int x, int y, int width, int height, const Color & color, float * texcoords );


void Movement::step( float delta_seconds )
{
	this->position.step(delta_seconds);
	this->position.current.x += (delta_seconds * this->velocity.x);
	this->position.current.y += (delta_seconds * this->velocity.y);
} // step

void Movement::tick( float step_alpha )
{
	this->position.interpolate(step_alpha);
} // tick

void InputMovement::step( float delta_seconds )
{
	const float MOVE_SPEED = 3.5;
	glm::vec2 & pos = this->position.current;
	pos.y -= input::state()->keyboard().is_down( input::KEY_W ) * MOVE_SPEED;
	pos.y += input::state()->keyboard().is_down( input::KEY_S ) * MOVE_SPEED;
	pos.x -= input::state()->keyboard().is_down( input::KEY_A ) * MOVE_SPEED;
	pos.x += input::state()->keyboard().is_down( input::KEY_D ) * MOVE_SPEED;
	
//	LOGV( "filtered: %g, delta: %g\n",  kernel::instance()->parameters().framedelta_raw, delta_seconds );
	
//	this->position.snap(pos);
	this->position.current = pos;
	
	this->position.step( delta_seconds );
} // step

void InputMovement::tick( float step_alpha )
{
//	LOGV( "step_alpha: %g\n", step_alpha );
	this->position.interpolate(step_alpha);
} // tick

Sprite::Sprite()
{
	this->layer = 0;
	this->hotspot_x = 0;
	this->hotspot_y = 0;
	this->rotation = 0;
	this->sprite_config = 0;
	this->scale = glm::vec2(1.0f, 1.0f);
	this->current_frame = 0;
	this->current_animation = 0;
} // Sprite

void Sprite::render( RenderControl & rc )
{
	glm::vec2 screen;
	
	if ( this->sprite_config )
	{
		Movement * movement = dynamic_cast<Movement*>(ComponentManager::component_matching_id( this->reference_id, MovementComponent ));
		if ( movement )
		{
			screen = movement->position.render;
		}
		
		InputMovement * im = dynamic_cast<InputMovement*>(ComponentManager::component_matching_id( this->reference_id, InputMovementComponent ));
		if ( im )
		{
			screen = im->position.render;
		}

	
		debugdraw::point( glm::vec3(screen, 0.0f), Color(255,255,255), this->scale.x*(this->sprite_config->collision_size/2.0f), 0.0f );

		assets::SpriteClip * clip = this->sprite_config->get_clip_by_index( current_animation );
		if (clip && clip->is_valid_frame(current_frame))
		{
//			screen.x = floor(screen.x);
//			screen.y = floor(screen.y);
			render_control.rs.rewind();
			render_control.rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
			render_control.rs.add_state( renderer::STATE_BLEND, 1 );
			rc.add_sprite_to_layer(0, screen.x, screen.y, scale.x*width, scale.y*height, color, clip->uvs_for_frame(current_frame));
//			render_control.rs.add_state( renderer::STATE_BLEND, 0 );
//			render_control.rs.run_commands();
			assets::Material * material = assets::materials()->find_with_id( this->material_id );
			render_control.render_stream( material );
		}
		else
		{
			LOGV( "clip is invalid or frame is invalid: %i\n", current_frame);
		}
	}
	else
	{
		LOGV( "no sprite config!\n" );
	}
} // render

void Sprite::step( float delta_seconds )
{
	this->animation_time -= delta_seconds;
	
	if ( this->animation_time <= 0 )
	{
		++current_frame;
		if ( this->sprite_config )
		{
			this->animation_time = sprite_config->frame_delay;
			
			assets::SpriteClip * clip = sprite_config->get_clip_by_index(current_animation);
			if ( clip )
			{
				if ( current_frame >= clip->total_frames )
				{
					// TODO: callback when this sequence finished?
					current_frame = 0;
				}
			}
		}
	}
} // step

void Sprite::tick( float step_alpha )
{
	glm::vec2 snap_pos;
	Emitter* emitter = 0;
	
	emitter = dynamic_cast<Emitter*>(ComponentManager::component_matching_id(this->reference_id, ParticleEmitterComponent));
	
	Movement* movement = dynamic_cast<Movement*>(ComponentManager::component_matching_id(this->reference_id, MovementComponent));
	if ( movement )
	{
		snap_pos = movement->position.render;
	}
	
	InputMovement* im = dynamic_cast<InputMovement*>(ComponentManager::component_matching_id(this->reference_id, InputMovementComponent));
	if ( im )
	{
		snap_pos = im->position.render;
	}
	
	if ( emitter && this->sprite_config )
	{
		glm::vec3 attachment = this->sprite_config->attachments[0];
		
		ParticleEmitter * e = emitter->emitter;
		if ( e )
		{
			glm::vec3 sprite_position = glm::vec3(snap_pos, 0);
			e->world_position.snap( sprite_position+attachment );
		}
	}
	
	constrain_to_screen();
} // tick


void Sprite::play_animation( const std::string & name )
{
	current_frame = 0;
	animation_time = 0;
	
	if ( sprite_config )
	{
		for( unsigned short i = 0; i < sprite_config->total_animations; ++i )
		{
			assets::SpriteClip * anim = this->sprite_config->get_clip_by_index(i);
			if ( name == anim->name )
			{
				current_animation = i;
				return;
			}
		}
	}
	
	LOGV( "unable to find animation: %s\n", name.c_str() );
} // play_animation

void Sprite::load_from_spriteconfig( assets::SpriteConfig *config )
{
	if ( !config )
	{
		return;
	}
	
	this->sprite_config = config;
	this->width = config->width;
	this->height = config->height;
	this->scale = config->scale;
	this->material_id = config->material_id;
} // load_from_spriteconfig


void Sprite::constrain_to_screen()
{
	float wx, wy;
	float hw = this->width / 2.0f;
	float hh = this->height / 2.0f;

	Movement * movement = dynamic_cast<Movement*>(ComponentManager::component_matching_id(this->reference_id, MovementComponent));
	if (movement)
	{
		wx = movement->position.render.x;
		wy = movement->position.render.y;
	}
	else
	{
		InputMovement * imovement = dynamic_cast<InputMovement*>(ComponentManager::component_matching_id(this->reference_id, InputMovementComponent));
		wx = imovement->position.render.x;
		wy = imovement->position.render.y;
	}
	
	float scaled_half_width = (hw * this->scale.x);
	float scaled_half_height = (hh * this->scale.y);
	
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
	

	
	if (movement)
	{
		movement->position.snap(glm::vec2(wx, wy));
	}
	else
	{
		InputMovement * imovement = dynamic_cast<InputMovement*>(ComponentManager::component_matching_id(this->reference_id, InputMovementComponent));
		if (imovement)
		{
			imovement->position.snap(glm::vec2(wx, wy));
		}
	}
}

ParticleSystem psys;



Emitter::Emitter()
{
	this->emitter = psys.add_emitter();
}

Emitter::~Emitter()
{
	psys.remove_emitter(this->emitter);
}

void Emitter::render( RenderControl & render_control )
{
	// render emitter at this layer
	render_control.render_emitter( this->emitter );
} // render

void Emitter::step( float delta_seconds )
{
	if ( this->emitter )
	{
		this->emitter->step(delta_seconds);
	}
} // step

void Emitter::tick( float step_alpha )
{
} // tick



void enemy_collision_handler(AABB2Collision* self, AABB2Collision* other)
{
	if (other->reference_id == 0)
	{
		// player hit this enemy
		LOGV("decrease energy\n");
		energy--;
	}
	else
	{
		 // increase score
		LOGV("increase score\n");
		score++;
	}
	
	self->set_flags(C_DISABLE);
	// entity message (all components): C_DISABLE
}

void player_collision_handler(AABB2Collision* self, AABB2Collision* other)
{

}

AABB2Collision::AABB2Collision()
{
	collision_mask = 0;
	collision_handler = 0;
}

AABB2Collision::~AABB2Collision()
{
	
}


bool AABB2Collision::collides_with( AABB2Collision * other ) const
{
	return false;
}

struct AABB2CollisionCheck
{
	AABB2Collision * object;
	AABB2 bounds;
};

void check_collision(IComponent* component, void* data)
{
	AABB2CollisionCheck * check = (AABB2CollisionCheck*)data;
	AABB2Collision * other = dynamic_cast<AABB2Collision*>(component);
	
	if ( check && other && check->object != other )
	{
		if ( (check->object->get_collision_mask() & other->get_collision_mask()) == 0 )
			return;

		AABB2 other_bounds;
		other->get_aabb(other_bounds);
		if (check->bounds.overlaps(other_bounds))
		{
			if (check->object->collision_handler)
			{
				check->object->collision_handler(check->object, other);
				if (other->collision_handler)
				{
					other->collision_handler(other, check->object);
				}
			}
		}
	}
}

void AABB2Collision::step( float dt_sec )
{
	AABB2CollisionCheck data;
	this->get_aabb(data.bounds);
	data.object = this;
	ComponentManager::for_each_component(PhysicsComponent, check_collision, &data);
}


void AABB2Collision::get_aabb( AABB2 & aabb ) const
{
	glm::vec2 pos;
	
	Movement * movement = dynamic_cast<Movement*>(ComponentManager::component_matching_id( this->reference_id, MovementComponent ));
	if ( movement )
	{
		pos = movement->position.render;
	}
	else
	{
		InputMovement * im = dynamic_cast<InputMovement*>(ComponentManager::component_matching_id( this->reference_id, InputMovementComponent ));
		if ( im )
		{
			pos = im->position.render;
		}
	}
	

	
	float hw = (this->box.x/2.0f);
	float hh = (this->box.y/2.0f);
	aabb.left = pos.x - hw;
	aabb.right = pos.x + hw;
	aabb.top = pos.y - hh;
	aabb.bottom = pos.y + hh;
}

unsigned short AABB2Collision::get_collision_mask() const
{
	return this->collision_mask;
}

void AABB2Collision::get_rotation( float & radians ) const
{
	
}


const int VIRTUAL_WIDTH = 800;
const int VIRTUAL_HEIGHT = 600;

void virtual_screen_to_pixels( float & tx, float & ty )
{
	kernel::Params & params = kernel::instance()->parameters();
	
	float mx = (params.render_width / (float)VIRTUAL_WIDTH);
	float my = (params.render_height / (float)VIRTUAL_HEIGHT);
	
	tx = mx * (VIRTUAL_WIDTH * tx);
	ty = my * (VIRTUAL_HEIGHT * ty);
}

void pixels_to_virtual_screen(float & px, float & py)
{
	kernel::Params & params = kernel::instance()->parameters();
	
	px = (px / params.render_width);
	py = (py / params.render_height);
}

GameScreen::GameScreen()
{
	Movement* pos = 0;
		
#if 1
	InputMovement* im = 0;
	im = dynamic_cast<InputMovement*>(ComponentManager::create_type( InputMovementComponent ));
	if ( im )
	{
		im->velocity = glm::vec2( 2.0f, 0.0f );
		im->position.snap( glm::vec2( 370.0f, 270.0f) );
		im->reference_id = 0;
	}
#else
	pos = dynamic_cast<Movement*>(ComponentManager::create_type( MovementComponent ));
	pos->velocity = glm::vec2( 150.0f, 0.0f );
	pos->position.snap(glm::vec2( -50.0f, 270.0f ));
	pos->reference_id = 0;
#endif
	
	assets::SpriteConfig * cfg = assets::sprites()->load_from_path("sprites/player");
	
	Sprite * spr = 0;
	spr = dynamic_cast<Sprite*>(ComponentManager::create_type(SpriteComponent));
	spr->reference_id = 0;	
	spr->load_from_spriteconfig(cfg);
	
	assets::SpriteConfig * enemy = assets::sprites()->load_from_path("sprites/enemy");
	spr = dynamic_cast<Sprite*>(ComponentManager::create_type(SpriteComponent));
	spr->reference_id = 1;
	spr->load_from_spriteconfig(enemy);
	

	pos = dynamic_cast<Movement*>(ComponentManager::create_type(MovementComponent));
	pos->position.snap(glm::vec2(100,230));
	pos->velocity = glm::vec2(0, 0);
	pos->reference_id = 1;
	
	
	
	// add collider component
	AABB2Collision * collision = dynamic_cast<AABB2Collision*>(ComponentManager::create_type(PhysicsComponent));
	if ( collision )
	{
		collision->reference_id = 0;
		collision->box = glm::vec2( 32.0f, 32.0f );
		collision->collision_mask = 1;
	}
	
	
	collision = dynamic_cast<AABB2Collision*>(ComponentManager::create_type(PhysicsComponent));
	if ( collision )
	{
		collision->reference_id = 1;
		collision->box = glm::vec2( 32.0f, 32.0f );
		collision->collision_mask = 1;
		collision->collision_handler = enemy_collision_handler;
	}
		
#if 0
	// add particle component
	assets::EmitterConfig * ecfg = assets::emitters()->load_from_path("sprites/exhaust");
	
	Emitter * emitter = 0;
	ParticleEmitter * e = 0;
	emitter = dynamic_cast<Emitter*>(ComponentManager::create_type(ParticleEmitterComponent));
	emitter->reference_id = 0;
	
	e = emitter->emitter;
	e->world_position.snap( glm::vec3( 350, 50, 0 ) );
	e->load_from_emitter_config(ecfg);
	
#if 0
	assets::Material * particle_material = assets::materials()->load_from_path("materials/particles2");
	if ( particle_material )
	{
		e->material_id = particle_material->Id();
	}
	
	e->init(128);
	e->next_spawn = 0;
	e->spawn_rate = 15;
	e->spawn_delay_seconds = 32;
	e->life.set_range(500.0f, 1000.0f);
	e->velocity.set_range(glm::vec3( -250.0f, -25.0f, 0.0f ), glm::vec3( -200.0f, 25.0f, 0.0f ));
	
	Color colors[] = { Color(255, 255, 255) };
	e->color_channel.create(1, colors, 1/3.0f);
	
	float alphas[] = {1.0f, 0.0f};
	e->alpha_channel.create(2, alphas, 1/1.0f);
	
	float sizes[] = {4.75f, 25.0f};
	e->size_channel.create(2, sizes, 1/1.0f);
#endif
#endif
	
	
//	EntityManager em;
//	EntityID a,b,c,d;
//	a = em.create();
//	b = em.create();
//	c = em.create();
//	d = -1;
//	em.remove(b);
//	d = em.create();
//	LOGV( "a: %i, b: %i, c: %i, d: %i\n", a, b, c, d );
	
	
	

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
	
	render_control.stream = &vb;
	render_control.attribs = test_attribs;
	
//	util::json_load_with_callback( "maps/test.json", tiled_map_loader, &tiled_map, true );
	
	// load map events
	util::json_load_with_callback( "maps/space.json", map_event_loader, &event_based_map, true );
	current_event = 0;
	current_gametime = 0;
	
	player_mat = assets::materials()->load_from_path("materials/player");
	
	assets::Material * background_material = assets::materials()->load_from_path("materials/background");
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
	
	

	
	// load sounds
	fire_delay = 200;
	next_fire = 0;
	player_fire = audio::create_sound("sounds/blaster1");
	
	enemy_explode = audio::create_sound( "sounds/enemy_explode1");
	
	this->default_shader.object = 0;
} // GameScreen


GameScreen::~GameScreen()
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
	
	ComponentManager::purge();
} // ~GameScreen


void GameScreen::render_layer( RenameThisData * layer )
{
#if 0
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
#endif
} // render_layer

void GameScreen::on_show( kernel::IApplication * app )
{
	LOGV( "GameScreen on show\n" );
} // on_show

void GameScreen::on_hide( kernel::IApplication * app )
{
	LOGV( "GameScreen on hide\n" );
} // on_hide


//void draw_movement(IComponent * component, void * data)
//{
//	Movement * mc = dynamic_cast<Movement*>(component);
//	if ( mc )
//	{
//		glm::vec2 & pos = mc->position.render;
//		debugdraw::point( glm::vec3(pos, 0.0f), Color(255,255,255) );
//	}
//}

void GameScreen::on_draw( kernel::IApplication * app )
{
	kernel::Params & params = kernel::instance()->parameters();
	
	// previously had floating point errors on android when this was only set during startup
	render_control.camera.ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -1.0f, 1.0f );
	
//	render_control.rs.rewind();
	
//	render_control.rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
//	render_control.rs.add_state( renderer::STATE_BLEND, 1 );
	
	ComponentManager::draw( render_control );
	
//	render_control.render_stream(player_mat);

#if 0
	// LAYER 0
	// draw background
	unsigned int num_rows = background_num_rows;
	for( unsigned int i = 0; i < num_rows; ++i )
	{
		render_layer( &background_layers[i * background_num_columns] );
	}
	render_vertexstream( camera, vb, rs, test_attribs, assets::materials()->load_from_path("materials/background") );
	vb.reset();
#endif
	
	
	
	// LAYER 1
	// draw all graphics objects
#if 0
	IGraphicObject * go = 0;
	ComponentManager::ComponentVector::iterator iter;
	ComponentManager::ComponentVector & graphic_objects = ComponentManager::component_list(RenderComponent);
	iter = graphic_objects.begin();
	for( ; iter != graphic_objects.end(); ++iter )
	{
		go = dynamic_cast<IGraphicObject*>(*iter);
		if ( go )
		{
			LOGV( "update graphic object!\n" );
			if ( go->layer_id() == 1 )
			{
				LOGV( "found layer id 1 graphic object\n" );
			}
		}
	}
#endif
	
	
//	ComponentManager::for_each_component(MovementComponent, draw_movement, 0);
	
	
	// LAYER 2
	// draw player related items
	
	
	// LAYER 3
	// draw all effects objects
//	render_particles( psys, renderer::driver(), render_control.camera.matCam, camera.matProj );
	
//	ParticleEmitter * e = psys.emitters[0];
//	glm::vec2 & rpos = player->position.render;
//	glm::vec3 emitter_pos = glm::vec3( rpos.x-28, rpos.y+8, 0 );
//	e->world_position.snap( emitter_pos );
//	rs.run_commands();
//	vb.reset();
	
	
	// LAYER 4
	// draw hud
	float tx = .02, ty = 0.05;
	virtual_screen_to_pixels( tx, ty );
	font::draw_string( font, tx, ty, xstr_format("Energy: %i", energy), Color(255,255,255));
	
	tx = 0.8;
	ty = 0.05;
	virtual_screen_to_pixels( tx, ty );
	font::draw_string( font, tx, ty, xstr_format("Score: %04i", score), Color(255,255,255));
	
	if ( game_state == GAME_FAIL )
	{
		float tx, ty;
		tx = 0.35;
		ty = 0.1;
		virtual_screen_to_pixels(tx, ty);
		font::draw_string( round_title, tx, ty, "Game Fail", Color(255,0,0) );
	}
	
	// LAYER N
	// debug primitives
	render_control.projection = glm::ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -0.1f, 128.0f );
	debugdraw::render( render_control.modelview, render_control.projection, params.render_width, params.render_height );
	
	
	
#if 0
	RenderContext context( rs, vb );
	
	// 3 - draw background
	unsigned int num_rows = background_num_rows;
	for( unsigned int i = 0; i < num_rows; ++i )
	{
		render_layer( &background_layers[i * background_num_columns] );
	}
	
	render_vertexstream( camera, vb, rs, test_attribs, assets::materials()->load_from_path("materials/background") );
	vb.reset();
	
	// 4 - draw other entities (enemies/powerups)
	Sprite * ent = 0;
	for( int i = 0; i < MAX_ENTITIES; ++i )
	{
		ent = &entities[i];
		if ( active_entities[i] )
		{
			//				ent->r_x = lerp( ent->last_world_x, ent->world_x, kernel::instance()->parameters().step_alpha );
			//				ent->r_y = lerp( ent->last_world_y, ent->world_y, kernel::instance()->parameters().step_alpha );
			ent->position.interpolate( kernel::instance()->parameters().step_alpha );
			
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
	
	
	
	ParticleEmitter * e = psys.emitters[0];
	
	glm::vec2 & rpos = player->position.render;
	glm::vec3 emitter_pos = glm::vec3( rpos.x-28, rpos.y+8, 0 );
	e->world_position.snap( emitter_pos );
	
	
	rs.run_commands();
	vb.reset();
	//		const char text[] = "---- Game ----";
	//		int center = (kernel::instance()->parameters().render_width / 2);
	//		int font_width = font::measure_width( font, text );
	//
	//		font::draw_string( font, center-(font_width/2), 40, text, Color(0,255,0) );
	
	
	//		debugdraw::text( 15, 55, "hello", Color(255,255,128) );
	
	
	//		debugdraw::line( glm::vec3(0, 0, 0), glm::vec3(player->r_x, player->r_y, 0), Color(255, 128, 0));
	
	render_particles( psys, renderer::driver(), camera.matCam, camera.matProj );
	
	
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
#endif
} // on_draw

void GameScreen::on_update( kernel::IApplication * app )
{
	if ( game_state == GAME_PLAY )
	{
		next_fire -= kernel::instance()->parameters().framedelta_filtered;
		
		if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
		{
			if (next_fire <= 0)
			{
				next_fire = fire_delay;
				
			}
		}
		
		current_gametime += (kernel::instance()->parameters().framedelta_filtered*.001);
	}
	
	ComponentManager::tick( kernel::instance()->parameters().step_alpha );
}

void GameScreen::on_step( kernel::IApplication * app )
{
	// update particles
	psys.step( kernel::instance()->parameters().step_interval_seconds );
	
	ComponentManager::step( kernel::instance()->parameters().step_interval_seconds );
} // on_step


void GameScreen::on_event( kernel::KeyboardEvent & event, kernel::IApplication * app )
{
	if (event.is_down)
	{
		if (event.key == input::KEY_ESCAPE)
		{
			kernel::instance()->set_active(false);
		}
	}
} // on_event (keyboard)
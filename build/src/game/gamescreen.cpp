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
#include "gamescreen.hpp"
#include "render_utilities.hpp"
#include "componentmanager.hpp"
#include "debugdraw.hpp"


using namespace render_utilities;


struct RenderContext
{
	RenderStream & rs;
	renderer::VertexStream & vb;
	
	RenderContext( RenderStream & inrs, renderer::VertexStream & invb ) : rs(inrs), vb(invb) {}
}; // RenderContext

class IGraphicObject : public virtual IComponent
{
public:
	IGraphicObject() : IComponent(GraphicComponent) {}
	virtual ComponentType component_type() const { return GraphicComponent; }
	
	virtual void render( RenderContext & context ) = 0;
	virtual void get_scale( float & x, float & y ) = 0;
	virtual Color get_color() = 0;
	virtual void update( float dt_sec ) = 0;
	
}; // IGraphicObject


class ICollisionObject : public virtual IComponent
{
public:
	ICollisionObject() : IComponent(PhysicsComponent) {}
	virtual ComponentType component_type() const { return PhysicsComponent; }
	
	virtual bool collides_with( ICollisionObject * other ) const = 0;
	virtual void world_position( float & x, float & y ) = 0;
	virtual void step( float dt_sec ) = 0;
	virtual void set_velocity( float x, float y ) = 0;
	virtual void get_aabb( AABB2 & aabb ) const = 0;
	virtual unsigned short get_collision_mask() const = 0;
	virtual void get_rotation( float & radians ) const = 0;
}; // ICollisionObject








class SpriteComponent : public virtual IGraphicObject
{
public:
	
	Color color;
	glm::vec2 scale;
	
	SpriteComponent() : IComponent(GraphicComponent) {}
	virtual void render( RenderContext & context );
	virtual void get_scale( float & x, float & y );
	virtual Color get_color();
	virtual void update( float delta_sec );
}; // SpriteComponent


void SpriteComponent::render(RenderContext &context)
{
	
} // render

void SpriteComponent::get_scale(float &x, float &y)
{
	x = scale.x;
	y = scale.y;
} // get_scale

Color SpriteComponent::get_color()
{
	return color;
} // get_color

void SpriteComponent::update( float delta_sec )
{
	
} // update




class AABB2Collision : public virtual ICollisionObject
{
public:
	virtual bool collides_with( ICollisionObject * other ) const;
	virtual void world_position( float & x, float & y );
	virtual void step( float dt_sec );
	virtual void set_velocity( float x, float y );
	virtual void get_aabb( AABB2 & aabb ) const;
	virtual unsigned short get_collision_mask() const;
	virtual void get_rotation( float & radians ) const;
};


bool AABB2Collision::collides_with( ICollisionObject * other ) const
{
	return false;
}

void AABB2Collision::world_position( float & x, float & y )
{
	
}

void AABB2Collision::step( float dt_sec )
{
	
}

void AABB2Collision::set_velocity( float x, float y )
{
	
}

void AABB2Collision::get_aabb( AABB2 & aabb ) const
{
	
}

unsigned short AABB2Collision::get_collision_mask() const
{
	return 0;
}

void AABB2Collision::get_rotation( float & radians ) const
{
	
}



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



void render_particles( ParticleSystem & ps, renderer::IRenderDriver * driver, glm::mat4 & modelview_matrix, glm::mat4 & projection_matrix )
{
	if ( ps.emitters.empty() )
	{
		// bail out if there are no emitters to render
		return;
	}
	
	renderer::VertexStream stream;
	
	stream.desc.add( renderer::VD_FLOAT3 );
	stream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
	stream.desc.add( renderer::VD_FLOAT2 );
	
	stream.create(1024, 1024, renderer::DRAW_INDEXED_TRIANGLES);
	ParticleEmitter * emitter = 0;
	
	renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
	glm::mat3 billboard = glm::transpose( glm::mat3(modelview_matrix) );
	
	ParticleEmitterVector::iterator iter = ps.emitters.begin();
	ParticleEmitterVector::iterator end = ps.emitters.end();
	
	for( ; iter != end; ++iter )
	{
		emitter = (*iter);
		emitter->world_position.interpolate( kernel::instance()->parameters().step_alpha );
		for( unsigned int p = 0; p < emitter->max_particles; ++p )
		{
			Particle * particle = &emitter->particle_list[ p ];
			if ( particle->life_remaining > 0 )
			{
				//
				particle->position.interpolate( kernel::instance()->parameters().step_alpha );
				if ( !stream.has_room(4, 6) )
				{
					LOGE( "particle stream is full - flush!\n" );
				}
				
				SpriteVertexType * v = (SpriteVertexType*)stream.request(4);
				glm::vec3 offset( particle->size, particle->size, 0 );
				glm::vec3 temp;
				
				temp = (billboard * -offset) + particle->position.render;
				v[0].x = temp.x;
				v[0].y = temp.y;
				v[0].z = temp.z;
				
				temp = (billboard * glm::vec3(offset[0], -offset[1], -offset[2])) + particle->position.render;
				v[1].x = temp.x;
				v[1].y = temp.y;
				v[1].z = temp.z;
				
				temp = (billboard * offset) + particle->position.render;
				v[2].x = temp.x;
				v[2].y = temp.y;
				v[2].z = temp.z;
				
				temp = (billboard * glm::vec3(-offset[0], offset[1], -offset[2])) + particle->position.render;
				v[3].x = temp.x;
				v[3].y = temp.y;
				v[3].z = temp.z;
				
				v[0].color = v[1].color = v[2].color = v[3].color = particle->color;
				
				v[0].u = 0; v[0].v = 0;
				v[1].u = 1; v[1].v = 0;
				v[2].u = 1; v[2].v = 1;
				v[3].u = 0; v[3].v = 1;
				
				//				debugdraw::point(particle->position.render, Color(255,255,255), particle->size, 0.0f);
				stream.append_indices( indices, 6 );
			}
		}
	}
	
	RenderStream rs;
	
	assets::ShaderString name("uv0");
	unsigned int test_attribs = assets::find_parameter_mask( name );
	
	name = "colors";
	test_attribs |= assets::find_parameter_mask(name);
	
	assets::Material * material = 0;
	assets::Shader * shader = 0;			
	emitter = ps.emitters[0];
	
	if ( emitter )
	{
		material = assets::material_by_id(emitter->material_id);
		shader = assets::find_compatible_shader( material->requirements + test_attribs );
	}

	
	glm::mat4 object_matrix;
	
	stream.update();
	
	rs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
	rs.add_state(renderer::STATE_BLEND, 1);
	rs.add_shader( shader );
	
	rs.add_state(renderer::STATE_DEPTH_TEST, 1);
	rs.add_state(renderer::STATE_DEPTH_WRITE, 0);
	
	rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &modelview_matrix );
	rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &projection_matrix );
	rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
	
	rs.add_material( material, shader );
	
	rs.add_draw_call( stream.vertexbuffer );
	
	
	rs.add_state(renderer::STATE_BLEND, 0);
	rs.add_state(renderer::STATE_DEPTH_WRITE, 1);
	rs.run_commands();
	
	stream.destroy();
} // render_particles


void render_vertexstream( Camera & camera, renderer::VertexStream & vb, RenderStream & rs, unsigned int attributes, assets::Material * material )
{
	long offset;
	rs.save_offset( offset );
	
	assert( material != 0 );
	
	vb.update();
	assets::Shader * shader = assets::find_compatible_shader( attributes + material->requirements );
	rs.add_shader( shader );
	
	glm::mat4 object_matrix;
	
	rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
	rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
	rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
	
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


GameScreen::GameScreen()
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

void GameScreen::on_show( kernel::IApplication * app )
{
	LOGV( "GameScreen on show\n" );
} // on_show

void GameScreen::on_hide( kernel::IApplication * app )
{
	LOGV( "GameScreen on hide\n" );
} // on_hide

void GameScreen::on_draw( kernel::IApplication * app )
{
	kernel::Params & params = kernel::instance()->parameters();
	
	// previously had floating point errors on android when this was only set during startup
	camera.ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -1.0f, 1.0f );
	
	rs.rewind();
	
	rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
	rs.add_state( renderer::STATE_BLEND, 1 );
	
	// setup render context
	RenderContext context( rs, vb );
	
	// LAYER 0
	// draw background
	unsigned int num_rows = background_num_rows;
	for( unsigned int i = 0; i < num_rows; ++i )
	{
		render_layer( &background_layers[i * background_num_columns] );
	}
	render_vertexstream( camera, vb, rs, test_attribs, assets::load_material("materials/background") );
	vb.reset();
	
	
	// LAYER 1
	// draw all graphics objects
	
	
	// LAYER 2
	// draw player related items
	
	
	// LAYER 3
	// draw all effects objects
	render_particles( psys, renderer::driver(), camera.matCam, camera.matProj );
	
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
	
	// LAYER N
	// debug primitives
	glm::mat4 modelview;
	glm::mat4 proj = glm::ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, -0.1f, 128.0f );
	debugdraw::render( modelview, proj, params.render_width, params.render_height );
	
	
	
#if 0
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
}

void GameScreen::on_step( kernel::IApplication * app )
{
	
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
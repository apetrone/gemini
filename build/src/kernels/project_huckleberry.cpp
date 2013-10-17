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
#include "kernel.hpp"
#include "input.hpp"
#include <stdio.h>
#include <slim/xlog.h>
#include "filesystem.hpp"
#include "assets/asset_mesh.hpp"
#include "camera.hpp"
#include "gemgl.hpp"
#include "debugdraw.hpp"

#include "renderer.hpp"
#include "renderstream.hpp"
#include "render_utilities.hpp"

#include "script.hpp"


enum EntityType
{
	Logic = 0,
	Model = 1,
	Sprite = 2
};

// script stuff
struct Entity
{
	HSQOBJECT instance;
	HSQOBJECT class_object;
	
	HSQOBJECT on_step;
	HSQOBJECT on_tick;
//	HSQOBJECT on_draw;
	
	glm::vec3 position;
	uint64_t id;
	std::string name;
	uint8_t type;
	
	Entity();
	~Entity();
	
	void step( float delta_seconds );
	void tick();
//	void draw();
	
	// bind functions for this object
	void bind_functions();
	
	// get/set functions for script interop
	inline const glm::vec3 & get_position() const { return position; }
	inline void set_position( const glm::vec3 & p ) { position = p; }
	const std::string & get_name() { return this->name; }
	void set_name( const std::string & object_name ) { this->name = object_name; }
	virtual void native_step( float delta_seconds );
	virtual void native_tick();
//	void native_draw();
}; // Entity

typedef std::vector<Entity*> EntityVector;



struct EntityList
{
	EntityVector objects;

	void add( Entity * object );
	void remove( Entity * object );
	void purge();
	Entity * find_with_name( const std::string & name );
	Entity * object_at_index( size_t index );
	size_t count() const;
}; // EntityList

void EntityList::add( Entity * object )
{
	this->objects.push_back( object );
} // add

void EntityList::remove( Entity * object )
{
	for (EntityVector::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
	{
		Entity * obj = (*it);
		
		if ( obj == object )
		{
			objects.erase( it );
			break;
		}
	}
} // remove

void EntityList::purge()
{
	objects.clear();
} // purge

Entity * EntityList::find_with_name( const std::string & name )
{
	for (EntityVector::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
	{
		Entity * obj = (*it);
		
		if ( name == obj->name )
		{
			return obj;
		}
	}
	
	return 0;
} // find_with_name

Entity * EntityList::object_at_index(size_t index)
{
	assert(index <= this->count());
	
	return objects[ index ];
} // object_at_index

size_t EntityList::count() const
{
	return objects.size();
} // count


EntityList & entity_list()
{
	static EntityList _entity_list;
	return _entity_list;
} // entity_list




Entity::Entity()
{
	this->type = Logic;
	this->id = entity_list().count();
	entity_list().add( this );
	LOGV( "Entity() - %p, %zu\n", this, this->id );
	
	sq_resetobject( &instance );
	sq_resetobject( &class_object );
	
	// Assumes the OT_INSTANCE is at position 1 in the stack
	SQRESULT res = sq_getstackobj( script::get_vm(), 1, &instance );
	script::check_result(res, "getstackobj");
	
	res = sq_getclass( script::get_vm(), 1 );
	script::check_result(res, "getclass" );
	
	res = sq_getstackobj( script::get_vm(), -1, &class_object );
	script::check_result(res, "getstackobj");
	
	// pop the OT_CLASS
	sq_poptop( script::get_vm() );
	
	this->bind_functions();
} // Entity

Entity::~Entity()
{
	LOGV( "~Entity() - %p, %zu\n", this, this->id );
	entity_list().remove( this );
} // ~Entity

void Entity::step( float delta_seconds )
{
	if ( sq_isnull(this->on_step) || sq_isnull(this->instance) )
	{
		return;
	}
	
	SQRESULT res;
	sq_pushobject( script::get_vm(), this->on_step );
	sq_pushobject( script::get_vm(), this->instance );
	sq_pushfloat( script::get_vm(), delta_seconds );
	res = sq_call( script::get_vm(), 2, SQFalse, SQTrue );
	
	sq_pop( script::get_vm(), 1 );
	if ( SQ_FAILED(res) )
	{
		script::check_result( res, "sq_call" );
		sq_pop( script::get_vm(), 1 );
	}
} // step

void Entity::tick()
{
	if ( sq_isnull(this->on_tick) || sq_isnull(this->instance) )
	{
		return;
	}

	SQRESULT res;
	sq_pushobject( script::get_vm(), this->on_tick );
	sq_pushobject( script::get_vm(), this->instance );
	res = sq_call( script::get_vm(), 1, SQFalse, SQTrue );
	
	sq_pop( script::get_vm(), 1 );
	if ( SQ_FAILED(res) )
	{
		script::check_result( res, "sq_call" );
		sq_pop( script::get_vm(), 1 );
	}
} // tick


void Entity::bind_functions()
{
	LOGV( "Entity::bind_functions: %p\n", this );
	this->on_tick = script::find_member( this->class_object, "tick" );
	this->on_step = script::find_member( this->class_object, "step" );
} // bind_functions

void Entity::native_step( float delta_seconds )
{
} // native_step

void Entity::native_tick()
{
//	LOGV( "Entity::native_tick\n" );
} // native_tick


// model entity
struct ModelEntity : public Entity
{
	assets::Mesh * mesh;
	glm::mat4 transform;
	
	ModelEntity();
	virtual ~ModelEntity() {}
	
	void set_model( const char * path );
	glm::mat4 get_transform() const { return this->transform; }
	void set_transform( const glm::mat4 & tr ) { this->transform = tr; }
}; // ModelEntity


ModelEntity::ModelEntity()
{
	this->type = Model;
	this->mesh = 0;
} // ModelEntity


void ModelEntity::set_model( const char * path )
{
	this->mesh = assets::meshes()->load_from_path( path );
	if ( this->mesh )
	{
		this->mesh->prepare_geometry();
	}
} // set_model

struct SpriteEntity : public Entity
{
	// these compose the 'animation state'
	unsigned short current_animation;	// currently active animation
	unsigned short current_frame;		// current frame of the animation
	float animation_time;				// current time of the animation
	
	// this is the 'stateless' part of the animation that we reference
	assets::SpriteConfig * sprite_config;
	
	unsigned int material_id;
	unsigned short width;
	unsigned short height;
	short hotspot_x;
	short hotspot_y;
	
	unsigned short layer;
	
	Color color;
	glm::vec2 scale;
	
	
	float rotation;
	
	render_utilities::PhysicsState<glm::vec2> world_position;
	glm::vec2 screen_origin;
	
	
	SpriteEntity();
	virtual ~SpriteEntity() {}
	
	virtual void native_step( float delta_seconds );
	virtual void native_tick();
	
	void set_sprite( const char * path );
	void play_animation( const char * name );
	glm::vec2 get_world_origin() const { return this->world_position.current; }
	void set_world_origin( const glm::vec2 & origin ) { this->world_position.snap( origin ); }
	
	glm::vec2 get_screen_origin() const { return this->screen_origin; }
	void set_screen_origin( const glm::vec2 & origin ) { this->screen_origin = origin; }
}; // SpriteEntity


SpriteEntity::SpriteEntity()
{
	this->type = Sprite;
	this->layer = 0;
	this->hotspot_x = 0;
	this->hotspot_y = 0;
	this->rotation = 0;
	this->sprite_config = 0;
	this->scale = glm::vec2(1.0f, 1.0f);
	this->current_frame = 0;
	this->current_animation = 0;
} // SpriteEntity

void SpriteEntity::native_step( float delta_seconds )
{
	this->world_position.step( delta_seconds );
	
//	this->world_position.current += glm::vec2( 5, 1 );
//	LOGV( "native_step SpriteEntity\n" );
}

void SpriteEntity::native_tick()
{
	this->world_position.interpolate( kernel::instance()->parameters().step_alpha );
//	LOGV( "native_tick SpriteEntity\n" );
}

void SpriteEntity::set_sprite( const char * path )
{
	this->sprite_config = assets::sprites()->load_from_path( path );
	if ( this->sprite_config )
	{
		this->width = this->sprite_config->width;
		this->height = this->sprite_config->height;
		this->scale = this->sprite_config->scale;
		this->material_id = this->sprite_config->material_id;
	}
	else
	{
		LOGV( "Unable to load %s\n", path );
	}
} // set_sprite

void SpriteEntity::play_animation( const char * name )
{
	current_frame = 0;
	animation_time = 0;
	
	if ( this->sprite_config )
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
	
	LOGV( "unable to find animation: %s\n", name );
} // play_animation

class ProjectHuckleberry : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
public:
	DECLARE_APPLICATION( ProjectHuckleberry );

	Camera camera;
	renderer::VertexStream sprite_stream;
	unsigned int sprite_attribs;
		
	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
		}
	}
	
	virtual void event( kernel::MouseEvent & event )
	{
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
			default: break;
		}
	}
	
	virtual void event( kernel::SystemEvent & event )
	{
		
	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 800;
		params.window_height = 600;
		params.window_title = "ProjectHuckleberry";
		return kernel::Application_Success;
	}
	
	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		Sqrat::RootTable root( script::get_vm() );
		
		// bind Entity to scripting language
		Sqrat::Class<Entity> entity( script::get_vm() );
		entity.Func( "tick", &Entity::native_tick );
		entity.Func( "step", &Entity::native_step );
//		entity.Func( "draw", &Entity::native_draw );
		entity.Var( "id", &Entity::id );
		entity.Prop( "name", &Entity::get_name, &Entity::set_name );
		entity.Prop( "position", &Entity::get_position, &Entity::set_position );
		root.Bind( "Entity", entity );
	
		Sqrat::DerivedClass<ModelEntity, Entity> model( script::get_vm() );
		model.Func( "set_model", &ModelEntity::set_model );
		model.Prop( "transform", &ModelEntity::get_transform, &ModelEntity::set_transform );
		root.Bind( "ModelEntity", model );
		
		
		Sqrat::DerivedClass<SpriteEntity, Entity> sprite( script::get_vm() );
		sprite.Func( "set_sprite", &SpriteEntity::set_sprite );
		sprite.Prop( "world_origin", &SpriteEntity::get_world_origin, &SpriteEntity::set_world_origin );
		sprite.Prop( "screen_origin", &SpriteEntity::get_screen_origin, &SpriteEntity::set_screen_origin );
		root.Bind( "SpriteEntity", sprite );
		
		script::execute_file("scripts/project_huckleberry.nut");
	
		debugdraw::startup(1024);
		
		// This is appropriate for drawing 3D models, but not sprites
//		camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
//		camera.yaw = -45;
//		camera.pitch = 30;
//		camera.update_view();
		
		// allocate sprite_stream
		assets::ShaderString name("uv0");
		sprite_attribs = 0;
		sprite_attribs |= assets::find_parameter_mask( name );
		
		name = "colors";
		sprite_attribs |= assets::find_parameter_mask( name );
		
		// setup the vertex stream: make sure we have enough vertices & indices
		// to accomodate the full background layer
		unsigned int max_vertices = (6 * 1024);
		unsigned int max_indices = (6 * 256);
		sprite_stream.reset();
		sprite_stream.desc.add( renderer::VD_FLOAT3 );
		sprite_stream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		sprite_stream.desc.add( renderer::VD_FLOAT2 );
		sprite_stream.create( max_vertices, max_indices, renderer::DRAW_INDEXED_TRIANGLES );
		
		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
		float dt = params.framedelta_filtered_msec * .001;
		debugdraw::update( params.framedelta_filtered_msec );
		
		camera.move_speed = 10.0f;
		
		if ( input::state()->keyboard().is_down(input::KEY_W) )
		{
			camera.move_forward( dt );
		}
		else if ( input::state()->keyboard().is_down(input::KEY_S) )
		{
			camera.move_backward( dt );
		}
		
		if ( input::state()->keyboard().is_down(input::KEY_A) )
		{
			camera.move_left( dt );
		}
		else if ( input::state()->keyboard().is_down(input::KEY_D) )
		{
			camera.move_right( dt );
		}
		
		// tick entities
		EntityVector::iterator it =	entity_list().objects.begin();
		EntityVector::iterator end = entity_list().objects.end();
		for( ; it != end; ++it )
		{
			(*it)->step( params.framedelta_filtered_msec * 0.001 );
		}
	}
	
	void add_sprite_to_layer( unsigned short layer, int x, int y, int width, int height, const Color & color, float * texcoords )
	{
		void add_sprite_to_stream( renderer::VertexStream & vb, int x, int y, int width, int height, const Color & color, float * texcoords );
		add_sprite_to_stream(sprite_stream, x, y, width, height, color, texcoords);
		sprite_stream.update();
	}
	
	void render_stream( RenderStream & rs, assets::Material * material, renderer::VertexStream & stream, Camera & camera )
	{
		//	long offset;
		//	rs.save_offset( offset );
		
		assert( material != 0 );
		
		assets::Shader * shader = assets::find_compatible_shader( sprite_attribs + material->requirements );
		
		assert( shader !=0 );
		rs.add_shader( shader );
		
		glm::mat4 object_matrix;
		
		rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
		rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
		rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
		
		rs.add_material( material, shader );
		rs.add_draw_call( stream.vertexbuffer );
		
		rs.run_commands();
		stream.reset();
		//	rs.load_offset( offset );
	} // render_stream
	
	void render_with_camera( Camera & camera )
	{
		RenderStream rs;
		
		renderer::GeneralParameters gp;
		
		gp.global_params = 0;
		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;
		
		rs.add_viewport(0, 0, 800, 600);
		rs.add_clearcolor(0.15f, 0.15f, 0.15f, 1.0f);
		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
//		rs.add_state(renderer::STATE_BACKFACE_CULLING, 1 );
//		rs.add_cullmode( renderer::CULLMODE_BACK );
		
//		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
//		glDisable( GL_POLYGON_OFFSET_LINE );
		for( EntityVector::iterator it = entity_list().objects.begin(); it != entity_list().objects.end(); ++it )
		{
			Entity * entity = (*it);
			if ( entity->type == Model )
			{
				ModelEntity * model = (ModelEntity*)entity;
				if ( model )
				{
					gp.object_matrix = &model->transform;
					assert( model->mesh != 0 );
					
					for( unsigned short i = 0; i < model->mesh->total_geometry; ++i )
					{
						render_utilities::stream_geometry( rs, &model->mesh->geometry[i], gp );
					}
				}
				
				
				rs.run_commands();
			}
			else if ( entity->type == Sprite )
			{
				SpriteEntity * sprite = (SpriteEntity*)entity;
				glm::vec2 screen = sprite->world_position.render;
				glm::vec2 & scale = sprite->scale;
				if ( sprite )
				{
					assets::SpriteClip * clip = sprite->sprite_config->get_clip_by_index( sprite->current_animation );
					if (clip && clip->is_valid_frame( sprite->current_frame ))
					{
						rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
						rs.add_state( renderer::STATE_BLEND, 1 );
						this->add_sprite_to_layer(0, screen.x, screen.y, scale.x*sprite->width, scale.y*sprite->height, sprite->color, clip->uvs_for_frame( sprite->current_frame ));
						assets::Material * material = assets::materials()->find_with_id( sprite->material_id );
						this->render_stream( rs, material, sprite_stream, camera );
					}
				}
			}
			
			rs.rewind();
		}

		
#if 0
		glEnable( GL_POLYGON_OFFSET_LINE );
		glPolygonOffset( -1, -1 );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		rs.rewind();
		for( SceneNodeVector::iterator it = nodes.begin(); it != nodes.end(); ++it )
		{
			SceneNode * node = (*it);
			assert( node->mesh != 0 );
			
			render_utilities::stream_geometry( rs, &node->mesh->geometry[0], gp );
		}
		
		rs.run_commands();
#endif
	
	} // render_with_camera
	
	virtual void tick( kernel::Params & params )
	{
		// tick entities
		EntityVector::iterator it =	entity_list().objects.begin();
		EntityVector::iterator end = entity_list().objects.end();
		for( ; it != end; ++it )
		{
			(*it)->tick();
		}
				
		camera.ortho( 0, params.render_width, params.render_height, 0, -0.1f, 128.0f );
//		camera.perspective( 60.0f, params.render_width, params.render_height, 0.1f, 128.0f );

		render_with_camera( camera );
	
//		debugdraw::text( 25, 50, xstr_format("camera.position = %g %g %g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255) );

//		debugdraw::axes( glm::mat4(1.0), 1.0f );
		
		debugdraw::render( camera.matCam, camera.matProj, params.render_width, params.render_height );
	}
	
	virtual void shutdown( kernel::Params & params )
	{
		debugdraw::shutdown();
	}
}; // ProjectHuckleberry

IMPLEMENT_APPLICATION( ProjectHuckleberry );
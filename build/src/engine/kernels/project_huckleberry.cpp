// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include "kernel.h"
#include "input.h"
#include <stdio.h>
#include <slim/xlog.h>
#include "filesystem.h"
#include "assets/asset_mesh.h"
#include "camera.h"
#include "gemgl.h"
#include "debugdraw.h"

#include "renderer.h"
#include "renderstream.h"
#include "render_utilities.h"
#include "particlesystem.h"
#include "script.h"


#include <Box2D/Box2D.h>
const float PIXELS_PER_METER = 32.0f;
#define PIXELS_TO_METER( x ) (x/PIXELS_PER_METER)
#define METERS_TO_PIXELS( x ) ((x) * PIXELS_PER_METER)

namespace physics
{



	struct PhysicsStep
	{
		uint32_t tick_count;

	};

	b2World * get_world();
	b2World * create_world( const b2Vec2 & gravity );
	void destroy_world( b2World * world );

	b2Body * create_body( b2World * world, b2BodyDef * body_def );
	void destroy_body( b2World * world, b2Body * body );

	void step( float delta_seconds );

	void debug_draw( b2World * world );


	class physics2d_debug_renderer : public b2Draw
	{
		/// Draw a closed polygon provided in CCW order.
		virtual void DrawPolygon(const b2Vec2* vertices, int vertexCount, const b2Color& color);

		/// Draw a solid closed polygon provided in CCW order.
		virtual void DrawSolidPolygon(const b2Vec2* vertices, int vertexCount, const b2Color& color);

		/// Draw a circle.
		virtual void DrawCircle(const b2Vec2& center, float radius, const b2Color& color);
		/// Draw a solid circle.
		virtual void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color);

		/// Draw a line segment.
		virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

		/// Draw a transform. Choose your own length scale.
		/// @param xf a transform.
		virtual void DrawTransform(const b2Transform& xf);
	};

	/// Draw a closed polygon provided in CCW order.
	void physics2d_debug_renderer::DrawPolygon(const b2Vec2* vertices, int vertexCount, const b2Color& color)
	{
		Color c( color.r );
		const b2Vec2 * last = 0;
		glm::vec3 start;
		glm::vec3 end;
		int v;
		const b2Vec2 * vertex = 0;
		for( v = 0; v < vertexCount; ++v )
		{
			vertex = &vertices[v];

			if ( last )
			{
				start = glm::vec3( int(METERS_TO_PIXELS( last->x )), int(METERS_TO_PIXELS( last->y )), 0 );
			}

			end = glm::vec3( int(METERS_TO_PIXELS( vertex->x )), int(METERS_TO_PIXELS( vertex->y )), 0 );

			if ( last )
			{
				debugdraw::line( start, end, c, 0 );
			}
			last = vertex;
		}

		// use the first vertex to close the loop
		vertex = &vertices[0];
		if ( last )
		{
			start = glm::vec3( int(METERS_TO_PIXELS( last->x )), int(METERS_TO_PIXELS( last->y )), 0 );
			end = glm::vec3( int(METERS_TO_PIXELS( vertex->x )), int(METERS_TO_PIXELS( vertex->y )), 0 );
			debugdraw::line( start, end, c, 0 );
		}
	}

	/// Draw a solid closed polygon provided in CCW order.
	void physics2d_debug_renderer::DrawSolidPolygon(const b2Vec2* vertices, int vertexCount, const b2Color& color)
	{
//		LOGV( "DrawSolidPolygon\n" );
	}

	/// Draw a circle.
	void physics2d_debug_renderer::DrawCircle(const b2Vec2& center, float radius, const b2Color& color)
	{
		LOGV( "DrawCircle\n" );
	}

	/// Draw a solid circle.
	void physics2d_debug_renderer::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color)
	{
		LOGV( "DrawSolidCircle\n" );
	}

	/// Draw a line segment.
	void physics2d_debug_renderer::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
	{
		Color c( color.r );
		debugdraw::line( glm::vec3(p1.x, p1.y, 0), glm::vec3(p2.x, p2.y, 0), c, 0 );
	}

	/// Draw a transform. Choose your own length scale.
	/// @param xf a transform.
	void physics2d_debug_renderer::DrawTransform(const b2Transform& xf)
	{
//		LOGV( "DrawTransform\n" );
	}
};


static b2Body * create_sprite_body( int width, int height, float x, float y, int flags = 0, int category = 0, int mask = 0 )
{
	b2BodyDef body_def;
	b2Body * body = 0;


	body_def.position.Set( PIXELS_TO_METER(x), PIXELS_TO_METER(y) );
	body_def.fixedRotation = false;
	body_def.allowSleep = true;

	// TODO: is this static?
	body_def.type = b2_dynamicBody;

	body = physics::create_body( physics::get_world(), &body_def );

	if ( body )
	{
		b2FixtureDef fixture;
		fixture.density = 1.0f;
		fixture.restitution = 0.1f;

		// TODO: is this a sensor?
		fixture.filter.categoryBits = category;
		fixture.filter.maskBits = mask;

		b2PolygonShape body_shape;
		body_shape.SetAsBox( PIXELS_TO_METER(width), PIXELS_TO_METER(height) );
		fixture.shape = &body_shape;
		body->CreateFixture( &fixture );

		// TODO: if needs fixed rotation
//		body->SetFixedRotation( true );


	}




	return body;
} // create_sprite_body

namespace physics
{
	b2World * _world = 0;

	b2World * get_world()
	{
		return _world;
	} // get_world

	b2World * create_world( const b2Vec2 & gravity )
	{
		if ( !_world )
		{
			_world = new b2World( gravity );
			_world->SetAutoClearForces( false );
		}

		return _world;
	} // create_world

	void destroy_world()
	{
		if ( _world )
		{
			delete _world;
			_world = 0;
		}
	} // destroy_world

	b2Body * create_body( b2World * world, b2BodyDef * body_def )
	{
		if ( world )
		{
			return world->CreateBody( body_def );
		}

		return 0;
	} // create_body

	void destroy_body( b2World * world, b2Body * body )
	{
		if ( world )
		{
			world->DestroyBody( body );
		}
	} // destroy_body

	void step( float delta_seconds )
	{

	} // step

	void debug_draw( b2World * world )
	{
		if ( world )
		{
			world->DrawDebugData();
		}
	} // debug_draw
};







struct SpriteVertexType
{
	float x, y, z;
	Color color;
	float u, v;
};

struct RenderGlobals
{
	renderer::VertexStream sprite_stream;
	RenderStream commands;
	Camera camera;

	size_t rendered_entities;
	unsigned int sprite_attribs;

	void setup()
	{
		// allocate sprite_stream
		assets::ShaderString name("uv0");
		sprite_attribs = 0;
		sprite_attribs |= assets::find_parameter_mask( name );

		name = "colors";
		sprite_attribs |= assets::find_parameter_mask( name );

		// setup the vertex stream
		unsigned int max_vertices = (6 * 1024);
		unsigned int max_indices = (6 * 256);
		sprite_stream.reset();
		sprite_stream.desc.add( renderer::VD_FLOAT3 );
		sprite_stream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		sprite_stream.desc.add( renderer::VD_FLOAT2 );
		sprite_stream.create( max_vertices, max_indices, renderer::DRAW_INDEXED_TRIANGLES );
	} // setup

	void render_stream( assets::Material * material, renderer::VertexStream & stream, Camera & camera )
	{
		assert( material != 0 );

		assets::Shader * shader = assets::find_compatible_shader( sprite_attribs + material->requirements );

		assert( shader !=0 );
		commands.add_shader( shader );

		glm::mat4 object_matrix;

		commands.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
		commands.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
		commands.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );

		commands.add_material( material, shader );
		commands.add_draw_call( stream.vertexbuffer );

		commands.run_commands();
		stream.reset();
	} // render_stream


	void render_emitter( ParticleEmitter * emitter, renderer::VertexStream & stream, Camera & camera )
	{
		if ( !emitter || (emitter && !emitter->emitter_config) )
		{
			return;
		}

		renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		glm::mat3 billboard = glm::transpose( glm::mat3(camera.matCam) );

		emitter->world_position.interpolate( kernel::instance()->parameters().step_alpha );

		assets::ShaderString name("uv0");
		unsigned int test_attribs = assets::find_parameter_mask( name );

		name = "colors";
		test_attribs |= assets::find_parameter_mask(name);

		assets::Material * material = 0;
		assets::Shader * shader = 0;

		material = assets::materials()->find_with_id(emitter->emitter_config->material_id);
		shader = assets::find_compatible_shader( material->requirements + test_attribs );

		glm::mat4 object_matrix;




		commands.rewind();
//		commands.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
//		commands.add_state(renderer::STATE_BLEND, 1);
		commands.add_shader( shader );

//		commands.add_state(renderer::STATE_DEPTH_TEST, 1);
//		commands.add_state(renderer::STATE_DEPTH_WRITE, 0);

		commands.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &camera.matCam );
		commands.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &camera.matProj );
		commands.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );

		commands.add_material( material, shader );

		for( unsigned int p = 0; p < emitter->emitter_config->max_particles; ++p )
		{
			Particle * particle = &emitter->particle_list[ p ];
			if ( particle->life_remaining > 0 )
			{
				particle->position.interpolate( kernel::instance()->parameters().step_alpha );
				if ( !stream.has_room(4, 6) )
				{
					// stream is full; need to flush stream here!
					stream.update();
					commands.add_draw_call( stream.vertexbuffer );
					commands.run_commands();
					commands.rewind();
					stream.reset();
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

		if (stream.last_vertex > 0)
		{
			stream.update();
			commands.add_draw_call( stream.vertexbuffer );
		}

//		commands.add_state(renderer::STATE_BLEND, 0);
//		commands.add_state(renderer::STATE_DEPTH_TEST, 0);
//		commands.add_state(renderer::STATE_DEPTH_WRITE, 1);
		commands.run_commands();
		stream.reset();
	} // render_emitter
}; // RenderGlobals


enum EntityType
{
	Logic,
	Model,
	Sprite,
	Emitter,
};




typedef std::vector<struct Entity*> EntityVector;

// script stuff
struct Entity
{
	enum Flags
	{
		EF_NONE = 0,
		EF_DELETE_INSTANCE,	// remove this instance
	};

	HSQOBJECT instance;
	HSQOBJECT class_object;

	HSQOBJECT on_step;
	HSQOBJECT on_tick;

	uint64_t id;
	std::string name;
	uint8_t type;
	uint32_t flags;

	Entity();
	virtual ~Entity();

	void step( float delta_seconds );
	void tick();

	// bind functions for this object
	void bind_functions();
	virtual void remove();

	// get/set functions for script interop
	const std::string & get_name() { return this->name; }
	void set_name( const std::string & object_name ) { this->name = object_name; }
	virtual void native_step( float delta_seconds );
	virtual void native_tick();
}; // Entity



using namespace Sqrat;
template<class C>
class EntityAllocator {

    static SQInteger setInstance(HSQUIRRELVM vm, C* instance)
    {
        sq_setinstanceup(vm, 1, instance);
        sq_setreleasehook(vm, 1, &Delete);
        return 0;
    }

    template <class T, bool b>
    struct NewC
    {
        T* p;
        NewC()
        {
			p = new T();
        }
    };

    template <class T>
    struct NewC<T, false>
    {
        T* p;
        NewC()
        {
			p = 0;
        }
    };

public:
    static SQInteger New(HSQUIRRELVM vm) {
        C* instance = NewC<C, is_default_constructible<C>::value >().p;
        setInstance(vm, instance);
        return 0;
    }

    template <int count>
    static SQInteger iNew(HSQUIRRELVM vm) {
        return New(vm);
    }

	// following New functions are used only if constructors are bound via Ctor() in class

    template <typename A1>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value
									 ));
    }
    template <typename A1,typename A2>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value
									 ));
    }
    template <typename A1,typename A2,typename A3>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        Var<A7> a7(vm, 8);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value,
									 a7.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7,typename A8>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        Var<A7> a7(vm, 8);
        Var<A8> a8(vm, 9);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value,
									 a7.value,
									 a8.value
									 ));
    }
    template <typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7,typename A8,typename A9>
    static SQInteger iNew(HSQUIRRELVM vm) {
        Var<A1> a1(vm, 2);
        Var<A2> a2(vm, 3);
        Var<A3> a3(vm, 4);
        Var<A4> a4(vm, 5);
        Var<A5> a5(vm, 6);
        Var<A6> a6(vm, 7);
        Var<A7> a7(vm, 8);
        Var<A8> a8(vm, 9);
        Var<A9> a9(vm, 10);
        if (Error::Instance().Occurred(vm)) {
            return sq_throwerror(vm, Error::Instance().Message(vm).c_str());
        }
        return setInstance(vm, new C(
									 a1.value,
									 a2.value,
									 a3.value,
									 a4.value,
									 a5.value,
									 a6.value,
									 a7.value,
									 a8.value,
									 a9.value
									 ));
    }

public:

    static SQInteger Copy(HSQUIRRELVM vm, SQInteger idx, const void* value) {
        C* instance = new C(*static_cast<const C*>(value));
        sq_setinstanceup(vm, idx, instance);
        sq_setreleasehook(vm, idx, &Delete);
        return 0;
    }

    static SQInteger Delete(SQUserPointer ptr, SQInteger size) {
		C * e = reinterpret_cast<C*>( ptr );
		e->flags |= Entity::EF_DELETE_INSTANCE;

		// this is quite a hack to get the system working well during a shutdown
		if ( !kernel::instance()->is_active() )
		{
//        C* instance = reinterpret_cast<C*>(ptr);
//        delete instance;
			delete e;
		}

        return 0;
    }
}; // EntityAllocator

template <class Type>
struct EntityList
{
	typedef std::vector<Type*> EntityVectorType;
	EntityVectorType objects;

	void add( Type * object )
	{
		this->objects.push_back( object );
	} // add

	virtual void remove( Type * object )
	{
		for (typename EntityVectorType::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
		{
			Type * obj = (*it);

			if ( obj == object )
			{
//				LOGV( "removing from entity list\n" );
				objects.erase( it );
				break;
			}
		}
	} // remove

	void clear()
	{
		objects.clear();
	} // clear

	void purge()
	{
		for (typename EntityVectorType::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
		{
			Entity * obj = (*it);
			delete obj;
		}

		clear();
	} // purge



	Type * find_with_name( const std::string & name )
	{
		for (typename EntityVectorType::iterator it = this->objects.begin(); it != this->objects.end(); ++it )
		{
			Entity * obj = (*it);

			if ( name == obj->name )
			{
				return obj;
			}
		}

		return 0;
	} // find_with_name


	Type * object_at_index( size_t index )
	{
		assert(index <= this->count());

		return objects[ index ];
	} // object_at_index

	size_t count() const
	{
		return objects.size();
	} // count
}; // EntityList

template <class Type>
EntityList<Type> & entity_list()
{
	static EntityList<Type> _entity_list;
	return _entity_list;
} // entity_list

Entity::Entity()
{

	this->type = Logic;
	this->id = entity_list<Entity>().count();
	this->flags = 0;
	entity_list<Entity>().add( this );
//	LOGV( "Entity() - %p, %zu\n", this, this->id );

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
//	LOGV( "~Entity() - %p, %zu\n", this, this->id );
//	entity_list<Entity>().remove( this );
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
//	LOGV( "Entity::bind_functions: %p\n", this );
	this->on_tick = script::find_member( this->class_object, "tick" );
	this->on_step = script::find_member( this->class_object, "step" );
} // bind_functions

void Entity::remove()
{
	this->flags = 1;
}

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



struct RenderableEntity : public Entity
{
	RenderableEntity * parent;
	uint8_t layer;

	EntityList<RenderableEntity>::EntityVectorType children;


	render_utilities::PhysicsState<glm::vec2> world_position;
	glm::vec2 velocity;


	virtual glm::vec2 get_position() const { return this->world_position.current; }
	virtual void set_position( const glm::vec2 & origin ) { this->world_position.current = origin; }

	virtual glm::vec2 get_velocity() const { return this->velocity; }
	virtual void set_velocity( const glm::vec2 & velocity ) { this->velocity = velocity; }


	RenderableEntity( RenderableEntity * parent );
	virtual ~RenderableEntity();

//	virtual void native_step( float delta_seconds );
//	virtual void native_tick();
	virtual void render( RenderGlobals & rg );
}; // RenderableEntity


RenderableEntity::RenderableEntity( RenderableEntity * parent )
{
	this->parent = parent;
	this->layer = 0;

	if ( this->parent )
	{
//		LOGV( "adding child node\n" );
		parent->children.push_back( this );
	}
	else
	{
//		LOGV( "adding renderableentity\n" );
		entity_list<RenderableEntity>().add( this );
	}
} // RenderableEntity

RenderableEntity::~RenderableEntity()
{
	if ( this->parent )
	{
//		LOGV( "removing child node: %p\n", this );
		for ( EntityList<RenderableEntity>::EntityVectorType::iterator it = parent->children.begin(); it != parent->children.end(); ++it )
		{
			if ( (*it) == this )
			{
				parent->children.erase( it );
				break;
			}
		}
	}
	else
	{
//		LOGV( "removing root renderable node\n" );
		entity_list<RenderableEntity>().remove( this );
	}
} // ~RenderableEntity

#if 0
void RenderableEntity::native_step( float delta_seconds )
{
	RenderableEntity * ent;
	for ( EntityList<RenderableEntity>::EntityVectorType::iterator it = children.begin(); it != children.end(); ++it )
	{
		ent = (*it);
		ent->step( delta_seconds );
	}
} // native_step

void RenderableEntity::native_tick()
{
	RenderableEntity * ent;
	for ( EntityList<RenderableEntity>::EntityVectorType::iterator it = children.begin(); it != children.end(); ++it )
	{
		ent = (*it);
		ent->tick();
	}
} // native_tick
#endif

void RenderableEntity::render( RenderGlobals & rg )
{
	EntityList<RenderableEntity>::EntityVectorType::iterator it;

	for( it = children.begin(); it != children.end(); ++it )
	{
		RenderableEntity * re = (*it);
		if ( re->flags == 0 )
		{
			re->render( rg );
			++rg.rendered_entities;
		}
	}
} // render



struct PhysicsEntity : public RenderableEntity
{
	b2Body * physics_body;
	unsigned int collision_group;
	unsigned int collision_mask;

	PhysicsEntity( RenderableEntity * parent = 0 );
	~PhysicsEntity();

	virtual glm::vec2 get_position() const
	{
		glm::vec2 position;

		if ( physics_body )
		{
			b2Vec2 pos = physics_body->GetPosition();
			position.x = METERS_TO_PIXELS(pos.x);
			position.y = METERS_TO_PIXELS(pos.y);
		}
		else
		{
			position = this->world_position.current;
		}

		return position;
	}

	virtual void set_position( const glm::vec2 & origin )
	{
		if ( physics_body )
		{
			b2Vec2 position;
			//			LOGV( "set position: %g, %g\n", origin.x, origin.y );
			position.x = PIXELS_TO_METER(origin.x);
			position.y = PIXELS_TO_METER(origin.y);

			physics_body->SetTransform( position, 0 );
			this->world_position.current = glm::vec2( position.x, position.y );
		}
		else
		{
			this->world_position.current = origin;
		}
	}

#if 0
	virtual glm::vec2 get_velocity() const
	{
		glm::vec2 velocity;
		if ( physics_body )
		{
			b2Vec2 vel = physics_body->GetLinearVelocity();
			velocity.x = METERS_TO_PIXELS( vel.x );
			velocity.y = METERS_TO_PIXELS( vel.y );
			return velocity;
		}

		return this->velocity;
	} // get_velocity

	virtual void set_velocity( const glm::vec2 & velocity )
	{
		if ( physics_body )
		{
			b2Vec2 v;
			v.x = PIXELS_TO_METER( velocity.x );
			v.y = PIXELS_TO_METER( velocity.y );
			physics_body->SetLinearVelocity( v );
		}
		else
		{
			this->velocity = velocity;
		}
	} // set_velocity
#endif

	virtual void on_collide( PhysicsEntity * other )
	{
		LOGV( "on_collide with %p\n", other );
	} // on_collide
}; // PhysicsEntity




PhysicsEntity::PhysicsEntity( RenderableEntity * parent ) : RenderableEntity( parent )
{
	this->physics_body = 0;
} // PhysicsEntity

PhysicsEntity::~PhysicsEntity()
{
	if ( physics_body )
	{
		physics::destroy_body( physics::get_world(), this->physics_body );
	}
} // ~PhysicsEntity


class PhysicsEntityContactListener : public b2ContactListener
{
	virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		LOGV( "PreSolve\n" );
		PhysicsEntity * a = (PhysicsEntity*)contact->GetFixtureA()->GetBody()->GetUserData();
		PhysicsEntity * b = (PhysicsEntity*)contact->GetFixtureB()->GetBody()->GetUserData();

		if ( a && b )
		{
			LOGV( "contact between %p and %p\n", a, b );
			contact->SetEnabled(false);
		}
	}

	virtual void BeginContact( b2Contact * contact )
	{
		LOGV( "BeginContact\n" );
		PhysicsEntity * a = (PhysicsEntity*)contact->GetFixtureA()->GetBody()->GetUserData();
		PhysicsEntity * b = (PhysicsEntity*)contact->GetFixtureB()->GetBody()->GetUserData();

		if ( a && b )
		{
			LOGV( "contact between %p and %p\n", a, b );
		}
	}

	virtual void EndContact( b2Contact * contact )
	{
		LOGV( "EndContact\n" );
	}
}; // PhysicsEntityContactListener

struct SpriteEntity : public PhysicsEntity
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

	Color color;
	glm::vec2 scale;

	SpriteEntity( RenderableEntity * parent = 0 );
	~SpriteEntity();

	virtual void native_step( float delta_seconds );
	virtual void native_tick();
	virtual void render( RenderGlobals & rg );

	void set_color( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a );

	void set_sprite( const char * path );
	void play_animation( const char * name );
	float get_width();
	float get_height();

	void add_sprite_to_layer( renderer::VertexStream & stream, unsigned short layer, int x, int y, int width, int height, const Color & color, float * texcoords );



}; // SpriteEntity


SpriteEntity::SpriteEntity( RenderableEntity * parent ) : PhysicsEntity( parent )
{
	this->type = Sprite;
	this->hotspot_x = 0;
	this->hotspot_y = 0;
	this->sprite_config = 0;
	this->scale = glm::vec2(1.0f, 1.0f);
	this->current_frame = 0;
	this->current_animation = 0;
	entity_list<SpriteEntity>().add( this );
} // SpriteEntity

SpriteEntity::~SpriteEntity()
{
	entity_list<SpriteEntity>().remove( this );
} // ~SpriteEntity

void SpriteEntity::native_step( float delta_seconds )
{
	this->world_position.step( delta_seconds );
//	LOGV( "native_step SpriteEntity\n" );
}

void SpriteEntity::native_tick()
{
	this->world_position.interpolate( kernel::instance()->parameters().step_alpha );
//	LOGV( "native_tick SpriteEntity\n" );
}

void SpriteEntity::render( RenderGlobals & rg )
{
	glm::vec2 screen;// = this->world_position.render;
	screen = this->get_position();
	screen.x = int( screen.x );
	screen.y = int( screen.y );
	glm::vec2 & scale = this->scale;

	if ( this->sprite_config )
	{
		assets::SpriteClip * clip = this->sprite_config->get_clip_by_index( this->current_animation );
		if (clip && clip->is_valid_frame( this->current_frame ))
		{
//			LOGV( "screen: %g, %g\n", screen.x, screen.y );
			this->add_sprite_to_layer(rg.sprite_stream, 0, screen.x, screen.y, scale.x*this->width, scale.y*this->height, this->color, clip->uvs_for_frame( this->current_frame ));
			assets::Material * material = assets::materials()->find_with_id( this->material_id );
			rg.render_stream( material, rg.sprite_stream, rg.camera );
			rg.commands.rewind();
		}
	}

	RenderableEntity::render( rg );
} // render

void SpriteEntity::set_sprite( const char * path )
{
	this->sprite_config = assets::sprites()->load_from_path( path );
	if ( this->sprite_config )
	{
		this->width = this->sprite_config->width;
		this->height = this->sprite_config->height;
		this->scale = this->sprite_config->scale;
		this->material_id = this->sprite_config->material_id;

		b2BodyDef body_def;

		collision_group = 2;
		collision_mask = 2;
		this->physics_body = create_sprite_body( this->sprite_config->collision_width, this->sprite_config->collision_height, 0, 0, 0, collision_group, collision_mask );
		this->physics_body->SetUserData( this );
	}
	else
	{
		LOGV( "Unable to load %s\n", path );
	}
} // set_sprite

void SpriteEntity::set_color( unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a )
{
	this->color.set( _r, _g, _b, _a );
} // set_color

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

float SpriteEntity::get_width()
{
	if ( !this->sprite_config )
	{
		return 0;
	}
	return (this->sprite_config->width * this->sprite_config->scale.x);
} // get_width

float SpriteEntity::get_height()
{
	if ( !this->sprite_config )
	{
		return 0;
	}
	return (this->sprite_config->height * this->sprite_config->scale.y);
} // get_height

void SpriteEntity::add_sprite_to_layer( renderer::VertexStream & stream, unsigned short layer, int x, int y, int width, int height, const Color & color, float * texcoords )
{
	void add_sprite_to_stream( renderer::VertexStream & vb, int x, int y, int width, int height, const Color & color, float * texcoords );
	add_sprite_to_stream(stream, x, y, width, height, color, texcoords);
	stream.update();
}

struct EmitterEntity : public RenderableEntity
{
	assets::EmitterConfig * emitter_config;
	ParticleEmitter * emitter;

	EmitterEntity( RenderableEntity * parent = 0 );
	virtual ~EmitterEntity();
	virtual void native_step( float delta_seconds );
	virtual void native_tick();
	virtual void render( RenderGlobals & rg );

	void set_emitter( const char * path );
};


EmitterEntity::EmitterEntity( RenderableEntity * parent ) : RenderableEntity( parent )
{
	this->type = Emitter;
	emitter_config = 0;
	emitter = 0;
} // EmitterEntity

EmitterEntity::~EmitterEntity()
{
	DESTROY( ParticleEmitter, this->emitter );
}

void EmitterEntity::native_step( float delta_seconds )
{
	if ( this->emitter_config )
	{
		this->emitter->world_position.snap( glm::vec3(this->world_position.render, 0.0f) );
		this->emitter->step( delta_seconds );
	}
} // native_step

void EmitterEntity::native_tick()
{

} // native_tick

void EmitterEntity::render( RenderGlobals & rg )
{
	rg.render_emitter( this->emitter, rg.sprite_stream, rg.camera );

	RenderableEntity::render( rg );
} // render

void EmitterEntity::set_emitter( const char * path )
{
	this->emitter_config = assets::emitters()->load_from_path( path );
	this->emitter = CREATE(ParticleEmitter);
	this->emitter->load_from_emitter_config( this->emitter_config );
} // set_emitter


struct GameRules
{
	HSQOBJECT instance;
	HSQOBJECT class_object;

	HSQOBJECT on_startup;
	HSQOBJECT on_tick;
	HSQOBJECT on_step;

	GameRules()
	{
		LOGV( "GameRules created.\n" );

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
	}

	void bind_functions()
	{
		this->on_startup = script::find_member( this->class_object, "startup" );
		this->on_tick = script::find_member( this->class_object, "tick" );
		this->on_step = script::find_member( this->class_object, "step" );
	}

	void native_startup() {}
	void native_tick() {}
	void native_step( float delta_seconds ) {}

	void startup()
	{
		if ( sq_isnull(this->on_startup) || sq_isnull(this->instance) )
		{
			return;
		}

		SQRESULT res;
		sq_pushobject( script::get_vm(), this->on_startup );
		sq_pushobject( script::get_vm(), this->instance );
		res = sq_call( script::get_vm(), 1, SQFalse, SQTrue );

		sq_pop( script::get_vm(), 1 );
		if ( SQ_FAILED(res) )
		{
			script::check_result( res, "sq_call" );
			sq_pop( script::get_vm(), 1 );
		}
	}

	void tick()
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
	}

	void step( float delta_seconds )
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
	}
}; // GameRules


struct Script_RenderInterface
{
	static uint32_t get_render_width()
	{
		return kernel::instance()->parameters().render_width;
	}

	static uint32_t get_render_height()
	{
		return kernel::instance()->parameters().render_height;
	}
}; // Script_RenderInterface



class ProjectHuckleberry : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
public:
	DECLARE_APPLICATION( ProjectHuckleberry );

	RenderGlobals rg;

	b2World * world;
	PhysicsEntityContactListener contact_listener;
	physics::physics2d_debug_renderer debug_renderer;

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

					rg.camera.move_view( event.mx-lastx, event.my-lasty );
				}
                break;
			}
			default: break;
		}
	}

	virtual void event( kernel::SystemEvent & event )
	{
		if ( event.subtype == kernel::WindowGainFocus )
		{
#if 0
			sq_pushroottable( script::get_vm() );
			sq_pushstring( script::get_vm(), "gamerules", -1 );
			sq_deleteslot( script::get_vm(), -2, false );
			sq_poptop( script::get_vm() );


			// purge all entities
			size_t num_ents = entity_list<Entity>().objects.size();
			LOGV( "ents still alive: %i\n", num_ents );

			entity_list<Entity>().purge();
			entity_list<RenderableEntity>().clear();

			// execute the script again
			script::execute_file("scripts/project_huckleberry.nut");

			// call startup on the new instance of gamerules
			{
				Sqrat::RootTable root( script::get_vm() );
				Sqrat::Object gamerules = root.GetSlot( "gamerules" );
				if ( !gamerules.IsNull() )
				{
					GameRules * gr = gamerules.Cast<GameRules*>();
					if ( gr )
					{
						gr->startup();
					}
				}
			}
#endif
		}
	}

	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 800;
		params.window_height = 600;
		params.window_title = "ProjectHuckleberry";
		return kernel::Application_Success;
	}

	void setup_script()
	{
		Sqrat::RootTable root( script::get_vm() );

		// bind Entity to scripting language
		Sqrat::Class<Entity, EntityAllocator<Entity> > entity( script::get_vm() );
		entity.Func( "tick", &Entity::native_tick );
		entity.Func( "step", &Entity::native_step );
		entity.Var( "id", &Entity::id );
		entity.Prop( "name", &Entity::get_name, &Entity::set_name );
		entity.Func( "remove", &Entity::remove );

		root.Bind( "Entity", entity );


		Sqrat::DerivedClass<ModelEntity, Entity, EntityAllocator<ModelEntity> > model( script::get_vm() );
		model.Func( "set_model", &ModelEntity::set_model );
		model.Prop( "transform", &ModelEntity::get_transform, &ModelEntity::set_transform );
		root.Bind( "ModelEntity", model );

		Sqrat::DerivedClass<RenderableEntity, Entity, EntityAllocator<RenderableEntity> > renderable( script::get_vm() );
		renderable.Ctor<RenderableEntity*>();
		renderable.Var( "layer", &RenderableEntity::layer );
		renderable.Prop( "position", &RenderableEntity::get_position, &RenderableEntity::set_position );
		renderable.Prop( "velocity", &RenderableEntity::get_velocity, &RenderableEntity::set_velocity );
		root.Bind( "RenderableEntity", renderable );

		Sqrat::DerivedClass<PhysicsEntity, RenderableEntity, EntityAllocator<PhysicsEntity> > phys( script::get_vm() );
		phys.Ctor<RenderableEntity*>();
		phys.Prop( "position", &PhysicsEntity::get_position, &PhysicsEntity::set_position );
//		phys.Prop( "velocity", &PhysicsEntity::get_velocity, &PhysicsEntity::set_velocity );
		root.Bind( "PhysicsEntity", phys );

		Sqrat::DerivedClass<SpriteEntity, RenderableEntity, EntityAllocator<SpriteEntity> > sprite( script::get_vm() );
		sprite.Ctor<RenderableEntity*>();
		sprite.Func( "set_sprite", &SpriteEntity::set_sprite );
		sprite.Func( "set_color", &SpriteEntity::set_color );
		sprite.Func( "width", &SpriteEntity::get_width );
		sprite.Func( "height", &SpriteEntity::get_height );
		root.Bind( "SpriteEntity", sprite );

		Sqrat::DerivedClass<EmitterEntity, RenderableEntity, EntityAllocator<EmitterEntity> > emitter( script::get_vm() );
		emitter.Ctor<RenderableEntity*>();
		emitter.Func( "set_emitter", &EmitterEntity::set_emitter );
		root.Bind( "EmitterEntity", emitter );

		Sqrat::Class<GameRules> gamerules( script::get_vm() );
		gamerules.Func( "startup", &GameRules::native_startup );
		gamerules.Func( "tick", &GameRules::native_tick );
		gamerules.Func( "step", &GameRules::native_step );
		root.Bind( "GameRules", gamerules );


		Sqrat::Class<Script_RenderInterface> render( script::get_vm() );
		render.StaticFunc( "width", &Script_RenderInterface::get_render_width );
		render.StaticFunc( "height", &Script_RenderInterface::get_render_height );
		root.Bind( "render", render );


		script::execute_file("scripts/project_huckleberry.nut");

		{
			Sqrat::Object gamerules = root.GetSlot( "gamerules" );
			if ( !gamerules.IsNull() )
			{
				GameRules * gr = gamerules.Cast<GameRules*>();
				if ( gr )
				{
					gr->startup();
				}
			}
		}
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		// physics world setup
		world = physics::create_world( b2Vec2( 0, 0 ) );
		world->SetContactListener( &contact_listener );
		world->SetDebugDraw( &debug_renderer );
		debug_renderer.SetFlags( b2Draw::e_aabbBit | b2Draw::e_centerOfMassBit | b2Draw::e_shapeBit );

		setup_script();






		debugdraw::startup(1024);

		// This is appropriate for drawing 3D models, but not sprites
//		camera.set_absolute_position( glm::vec3(8, 5, 8.0f) );
//		camera.yaw = -45;
//		camera.pitch = 30;
//		camera.update_view();

		rg.setup();


		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		float delta_seconds = params.framedelta_filtered_msec * .001;
		debugdraw::update( params.framedelta_filtered_msec );

		rg.camera.move_speed = 10.0f;

		if ( input::state()->keyboard().is_down(input::KEY_W) )
		{
			rg.camera.move_forward( delta_seconds );
		}
		else if ( input::state()->keyboard().is_down(input::KEY_S) )
		{
			rg.camera.move_backward( delta_seconds );
		}

		if ( input::state()->keyboard().is_down(input::KEY_A) )
		{
			rg.camera.move_left( delta_seconds );
		}
		else if ( input::state()->keyboard().is_down(input::KEY_D) )
		{
			rg.camera.move_right( delta_seconds );
		}

		if ( world )
		{
			world->Step( params.step_interval_seconds, 2, 1 );
			world->ClearForces();
		}

		Sqrat::RootTable root( script::get_vm() );
		Sqrat::Object gamerules = root.GetSlot( "gamerules" );
		if ( !gamerules.IsNull() )
		{
			GameRules * gr = gamerules.Cast<GameRules*>();
			if ( gr )
			{
				gr->step( kernel::instance()->parameters().step_interval_seconds );
			}
		}

		// tick entities
		EntityVector::iterator it =	entity_list<Entity>().objects.begin();
		EntityVector::iterator end = entity_list<Entity>().objects.end();
		for( ; it != end; ++it )
		{
			(*it)->step( kernel::instance()->parameters().step_interval_seconds );
		}
	}

	struct sort_sprite_layer_descending
	{
		bool operator() (RenderableEntity * left, RenderableEntity * right)
		{
			return left->layer > right->layer;
		}
	}; // sort_sprite_layer_descending

	void render_with_camera( Camera & camera, uint32_t render_width, uint32_t render_height )
	{
//		renderer::GeneralParameters gp;
//		gp.global_params = 0;
//		gp.camera_position = &camera.pos;
//		gp.modelview_matrix = &camera.matCam;
//		gp.projection_project = &camera.matProj;

		RenderStream crs( 128, 64 );

		crs.add_viewport(0, 0, render_width, render_height);
		crs.add_clearcolor(0.15f, 0.15f, 0.15f, 1.0f);
		crs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );

		crs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
		crs.add_state(renderer::STATE_BLEND, 1);
		crs.add_state(renderer::STATE_DEPTH_TEST, 0);
		crs.run_commands();

//		rs.add_state(renderer::STATE_BACKFACE_CULLING, 1 );
//		rs.add_cullmode( renderer::CULLMODE_BACK );

//		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
//		glDisable( GL_POLYGON_OFFSET_LINE );

		// sort ents by layer.
		std::sort( entity_list<RenderableEntity>().objects.begin(), entity_list<RenderableEntity>().objects.end(), sort_sprite_layer_descending() );

		rg.rendered_entities = 0;
		for( EntityList<RenderableEntity>::EntityVectorType::iterator it = entity_list<RenderableEntity>().objects.begin();
			it != entity_list<RenderableEntity>().objects.end(); ++it )
		{
			RenderableEntity * re = (*it);
			if ( re->flags == 0 )
			{
				re->render( rg );
				++rg.rendered_entities;
			}
		}

		//LOGV( "rendered_entities: %i\n", rg.rendered_entities );

#if 0
		for( EntityVector::iterator it = entity_list<Entity>().objects.begin(); it != entity_list<Entity>().objects.end(); ++it )
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

		}
#endif
		crs.rewind();
		crs.add_state( renderer::STATE_BLEND, 0 );
		crs.add_state(renderer::STATE_DEPTH_TEST, 1);
		crs.run_commands();
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

	void deferred_delete( bool only_deferred )
	{
		// trim entities flagged for removal
		EntityVector::iterator it = entity_list<Entity>().objects.begin();
		EntityVector::iterator end = entity_list<Entity>().objects.end();
		for( ; it != end; ++it )
		{
			Entity * ent = (*it);
			if ((only_deferred && (ent->flags & Entity::EF_DELETE_INSTANCE)) || !only_deferred )
			{
//				LOGV( "removing flagged entity: %p\n", ent );
				it = entity_list<Entity>().objects.erase( it );
//				ent->flags &= ~Entity::EF_DELETE_INSTANCE;
				delete ent;
			}
		}
	}

	virtual void tick( kernel::Params & params )
	{
		Sqrat::RootTable root( script::get_vm() );
		Sqrat::Object gamerules = root.GetSlot( "gamerules" );
		if ( !gamerules.IsNull() )
		{
			GameRules * gr = gamerules.Cast<GameRules*>();
			if ( gr )
			{
				gr->tick();
			}
		}

		// tick entities
		EntityVector::iterator it =	entity_list<Entity>().objects.begin();
		EntityVector::iterator end = entity_list<Entity>().objects.end();
		Entity * ent;
		for( ; it != end; ++it )
		{
			ent = (*it);
			if ( !(ent->flags & Entity::EF_DELETE_INSTANCE) )
			{
				(*it)->tick();
			}
		}

		deferred_delete( true );



		rg.camera.ortho( 0, params.render_width, params.render_height, 0, -0.1f, 128.0f );
//		camera.perspective( 60.0f, params.render_width, params.render_height, 0.1f, 128.0f );

		render_with_camera( rg.camera, params.render_width, params.render_height );

//		debugdraw::text( 25, 50, xstr_format("camera.position = %g %g %g", camera.pos.x, camera.pos.y, camera.pos.z), Color(255, 255, 255) );

//		debugdraw::axes( glm::mat4(1.0), 1.0f );


		physics::debug_draw( world );

		debugdraw::render( rg.camera.matCam, rg.camera.matProj, params.render_width, params.render_height );
	}

	virtual void shutdown( kernel::Params & params )
	{
		physics::destroy_world();
		debugdraw::shutdown();
	}
}; // ProjectHuckleberry

IMPLEMENT_APPLICATION( ProjectHuckleberry );

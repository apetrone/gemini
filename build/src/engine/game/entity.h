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

#include <slim/xlog.h>

#include <platform/mem.h>

#include <sqrat.h>

#include "script.h"

#include "kernel.h"

#include "assets/asset_mesh.h"
#include "scene_graph.h"
#include "physics.h"

//typedef std::vector< struct Entity*, GeminiAllocator<struct Entity*> > EntityVector;


const SQChar ENTITY_START_NAME[] = _SC("Start");
const SQChar ENTITY_UPDATE_NAME[] = _SC("Update");
const SQChar ENTITY_FIXED_UPDATE_NAME[] = _SC("FixedUpdate");
const SQChar ENTITY_REMOVE_NAME[] = _SC("Remove");

struct Entity;

class EntityMotionInterface : public physics::PhysicsMotionInterface
{
	Entity* target;
	scenegraph::Node* node;
	
public:
	
	EntityMotionInterface(Entity* entity, scenegraph::Node* sgnode) : target(entity), node(sgnode)
	{
	}
	
	virtual void get_transform(glm::vec3& position, const glm::quat& orientation);
	
	virtual void set_transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& mass_center_offset);
};



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
	
	HSQOBJECT on_fixed_update;
	HSQOBJECT on_update;
	
	HSQOBJECT on_collision_began;
	HSQOBJECT on_collision_ended;

	uint64_t id;
	uint32_t flags;
	
	String name;
	
	Entity();
	virtual ~Entity();
	EntityMotionInterface* motion_interface;
	
	void fixed_update(float delta_seconds);
	void update();
	
	// bind functions for this object
	void bind_functions();
	virtual void remove();
	
	// get/set functions for script interop
	const String & get_name() { return this->name; }
	void set_name( const String & object_name ) { this->name = object_name; }
	virtual void native_fixed_update( float delta_seconds );
	virtual void native_update();
	virtual void native_collision_began();
	virtual void native_collision_ended();
	
	void collision_began();
	void collision_ended();
	
	glm::vec3 position;
	void set_position(glm::vec3* new_position);
	glm::vec3* get_position();
	void attach_camera(Camera* camera);
	
	void apply_force(glm::vec3* force, glm::vec3* local_position);
	void apply_central_force(glm::vec3* force);
	void set_mass(float mass);
	
	assets::Mesh* mesh;
	scenegraph::Node* node;
	
	// perhaps move these into a unified interface?
	physics::CollisionObject* collision_object;
		
	// functions for this script object
	void set_model(const char* path);
	
	void set_physics(int physics_type);
}; // Entity

struct Trigger : public Entity
{
};


struct GameRules
{
	HSQOBJECT instance;
	HSQOBJECT class_object;
	
	HSQOBJECT on_start;
	HSQOBJECT on_update;
	HSQOBJECT on_fixed_update;
	
	Camera* active_camera;
	
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
		active_camera = nullptr;
	}
	
	void bind_functions()
	{
		this->on_start = script::find_member( this->class_object, ENTITY_START_NAME );
		this->on_update = script::find_member( this->class_object, ENTITY_UPDATE_NAME );
		this->on_fixed_update = script::find_member( this->class_object, ENTITY_FIXED_UPDATE_NAME );
	}
	
	void native_start() {}
	void native_update() {}
	void native_fixed_update( float delta_seconds ) {}
	void set_active_camera(Camera* camera)
	{
		active_camera = camera;
	}
	
	Camera* get_active_camera() const { return active_camera; }
	
	void start()
	{
		if ( sq_isnull(this->on_start) || sq_isnull(this->instance) )
		{
			return;
		}
		
		SQRESULT res;
		sq_pushobject( script::get_vm(), this->on_start );
		sq_pushobject( script::get_vm(), this->instance );
		res = sq_call( script::get_vm(), 1, SQFalse, SQTrue );
		
		sq_pop( script::get_vm(), 1 );
		if ( SQ_FAILED(res) )
		{
			script::check_result( res, "sq_call" );
			sq_pop( script::get_vm(), 1 );
		}
	}
	
	void fixed_update(float delta_seconds)
	{
		if ( sq_isnull(this->on_fixed_update) || sq_isnull(this->instance) )
		{
			return;
		}
		
		SQRESULT res;
		sq_pushobject( script::get_vm(), this->on_fixed_update );
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
	
	void update()
	{
		if ( sq_isnull(this->on_update) || sq_isnull(this->instance) )
		{
			return;
		}
		
		SQRESULT res;
		sq_pushobject( script::get_vm(), this->on_update );
		sq_pushobject( script::get_vm(), this->instance );
		res = sq_call( script::get_vm(), 1, SQFalse, SQTrue );
		
		sq_pop( script::get_vm(), 1 );
		if ( SQ_FAILED(res) )
		{
			script::check_result( res, "sq_call" );
			sq_pop( script::get_vm(), 1 );
		}
	}
}; // GameRules



void entity_startup();
void entity_post_script_load();
void entity_shutdown();
void entity_step();
void entity_tick();
void entity_set_scene_root(scenegraph::Node* root);
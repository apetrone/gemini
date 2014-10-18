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
#include <gemini/mem.h>

#include "entity.h"
#include "entity_allocator.h"
#include "entity_list.h"
#include "physics.h"




void entity_startup()
{
	Sqrat::RootTable root( script::get_vm() );
	
	// bind Entity to scripting language
	Sqrat::Class<Entity, EntityAllocator<Entity> > entity( script::get_vm() );
	entity.Var( "id", &Entity::id );
	entity.Prop( "name", &Entity::get_name, &Entity::set_name );
	
	entity.Func(ENTITY_UPDATE_NAME, &Entity::native_update);
	entity.Func(ENTITY_FIXED_UPDATE_NAME, &Entity::native_fixed_update);
	entity.Func(ENTITY_REMOVE_NAME, &Entity::remove );
	
	// for now, all these are going to be bolted onto the Entity class
	entity.Func("SetModel", &Entity::set_model);
	
	entity.Prop("position", &Entity::get_position, &Entity::set_position);
//	entity.Prop("rotation", &Entity::get_rotation, &Entity::set_rotation);
	
	
	
	root.Bind( "Entity", entity );
	
	// gamerules
	Sqrat::Class<GameRules> gamerules( script::get_vm() );
	gamerules.Func(ENTITY_START_NAME, &GameRules::native_start );
	gamerules.Func(ENTITY_UPDATE_NAME, &GameRules::native_update );
	gamerules.Func(ENTITY_FIXED_UPDATE_NAME, &GameRules::native_fixed_update );
	root.Bind( "GameRules", gamerules );
	

}

void entity_post_script_load()
{
	// execute script?
	Sqrat::RootTable root( script::get_vm() );
	Sqrat::Object gamerules = root.GetSlot( "gamerules" );
	if ( !gamerules.IsNull() )
	{
		GameRules * gr = gamerules.Cast<GameRules*>();
		if ( gr )
		{
			gr->start();
		}
	}
}


void entity_prestep()
{
	// this updates the ent's position with that of the physics body
	EntityVector::iterator it =	entity_list().objects.begin();
	EntityVector::iterator end = entity_list().objects.end();
	Entity* ent;
	for( ; it != end; ++it )
	{
		ent = (*it);
		if (ent->body)
		{
			// copy body's position over to node and entity
			const glm::vec3& world_position = ent->body->get_world_position();
			ent->node->translation = world_position;
			ent->position = world_position;
		}
	}
}

void entity_step()
{
	entity_prestep();
	
	Sqrat::RootTable root( script::get_vm() );
	Sqrat::Object gamerules = root.GetSlot( "gamerules" );
	if ( !gamerules.IsNull() )
	{
		GameRules * gr = gamerules.Cast<GameRules*>();
		if ( gr )
		{
			gr->fixed_update( kernel::instance()->parameters().step_interval_seconds );
		}
	}
	
	
	
	// step entities
	EntityVector::iterator it =	entity_list().objects.begin();
	EntityVector::iterator end = entity_list().objects.end();
	for( ; it != end; ++it )
	{
		(*it)->fixed_update( kernel::instance()->parameters().step_interval_seconds );
		
		Entity* ent = (*it);
		// update render node and body
		if (ent->node && ent->body)
		{
			ent->node->translation = ent->position;
			ent->body->set_world_position(ent->position);
		}
	}
	
	
}

void entity_deferred_delete( bool only_deferred )
{
	// trim entities flagged for removal
	EntityVector::iterator it = entity_list().objects.begin();
	EntityVector::iterator end = entity_list().objects.end();
	for( ; it != end; ++it )
	{
		Entity * ent = (*it);
		if ((only_deferred && (ent->flags & Entity::EF_DELETE_INSTANCE)) || !only_deferred )
		{
//			LOGV( "removing flagged entity: %p\n", ent );
			it = entity_list().objects.erase( it );
//			ent->flags &= ~Entity::EF_DELETE_INSTANCE;
			delete ent;
		}
	}
}

void entity_tick()
{
	Sqrat::RootTable root( script::get_vm() );
	Sqrat::Object gamerules = root.GetSlot( "gamerules" );
	if ( !gamerules.IsNull() )
	{
		GameRules * gr = gamerules.Cast<GameRules*>();
		if ( gr )
		{
			gr->update();
		}
	}
	
	// tick entities
	EntityVector::iterator it =	entity_list().objects.begin();
	EntityVector::iterator end = entity_list().objects.end();
	Entity * ent;
	for( ; it != end; ++it )
	{
		ent = (*it);
		if ( !(ent->flags & Entity::EF_DELETE_INSTANCE) )
		{
			(*it)->update();
		}
	}
	
	entity_deferred_delete( true );
	
}

void entity_set_scene_root(scenegraph::Node* root)
{
	set_entity_root(root);
}

void entity_shutdown()
{
	LOGV("entity_shutdown!\n");
	set_entity_root(nullptr);
	entity_list().clear();
}


Entity::Entity()
{
	this->id = entity_list().count();
	this->flags = 0;
//	this->mesh = 0;
	this->node = 0;
	this->body = 0;
	
	entity_list().add( this );
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
	LOGV( "~Entity() - %p, %zu\n", this, this->id );
	entity_list().remove( this );
	
	if (this->body)
	{
		DESTROY(RigidBody, this->body);
	}
} // ~Entity

void Entity::fixed_update( float delta_seconds )
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
} // step

void Entity::update()
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
} // tick


void Entity::bind_functions()
{
	//	LOGV( "Entity::bind_functions: %p\n", this );

	this->on_fixed_update = script::find_member( this->class_object, ENTITY_FIXED_UPDATE_NAME );
	this->on_update = script::find_member( this->class_object, ENTITY_UPDATE_NAME );
} // bind_functions

void Entity::remove()
{
	this->flags = 1;
}

void Entity::native_fixed_update( float delta_seconds )
{
} // native_fixed_update

void Entity::native_update()
{
	//	LOGV( "Entity::native_tick\n" );
} // native_update

void Entity::set_position(glm::vec3 *new_position)
{
	position = *new_position;
	
	if (body)
	{
		body->set_world_position(*new_position);
	}
}

glm::vec3* Entity::get_position()
{
	return &position;
}

void load_mesh(scenegraph::Node* root, const char* path, scenegraph::Node*& node, physics::RigidBody*& body, bool build_physics_from_mesh = true)
{
	// 1. load the mesh from file
	assets::Mesh* mesh = assets::meshes()->load_from_path(path);
	if (mesh)
	{
		mesh->prepare_geometry();
		
		// clone the hierarchy to the renderable scene
		node = clone_to_scene(mesh->scene_root, root);
	}
	else
	{
		LOGW("Unable to load model: %s\n", path);
	}

	// 2. if physics is to be used; generate physics body from mesh
	if (mesh && build_physics_from_mesh)
	{
		float mass_kg = 0.0f;
		body = physics::create_physics_for_mesh(mesh, mass_kg);
	}
}

void Entity::set_model(const char* path)
{
	load_mesh(get_entity_root(), path, this->node, this->body, true);
	
}
// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <platform/mem.h>

#include <core/logging.h>

#include "entity.h"
#include "entity_list.h"
//#include "physics/physics.h"
//#include "camera.h"
//#include "kernel.h"
#include <sdk/physics_rigidbody.h>

// TODO: this should not bring in the character controller!
//#include "charactercontroller.h"

//#include "scene_graph.h"
//#include "assets/asset_mesh.h"


#include <sdk/entity_api.h>
#include <sdk/engine_api.h>
#include <sdk/physics_api.h>
#include <sdk/model_api.h>




using namespace gemini;
using namespace gemini::physics;

static void entity_collision_callback(CollisionEventType type, ICollisionObject* first, ICollisionObject* second)
{
	assert(first != 0);
	assert(second != 0);
	
	Entity* ent0 = static_cast<Entity*>(first->get_user_data());
	Entity* ent1 = static_cast<Entity*>(second->get_user_data());
	
	if (ent0 && ent1)
	{
		if (type == Collision_Began)
		{
			ent0->collision_began(ent1);
//			ent1->collision_began(ent0);
		}
		else if (type == Collision_Ended)
		{
			ent0->collision_ended(ent1);
//			ent1->collision_ended(ent0);
		}
	}
}

void entity_startup()
{
}

void entity_post_script_load()
{
}

void entity_deferred_delete( bool only_deferred )
{
	// trim entities flagged for removal
	EntityListType::Collection::iterator it = entity_list().objects.begin();
	EntityListType::Collection::iterator end = entity_list().objects.end();
	for( ; it != end; )
	{
		Entity * ent = (*it);
		if ((only_deferred && (ent->flags & Entity::EF_DELETE_INSTANCE)) || !only_deferred )
		{
//			LOGV( "removing flagged entity: %p\n", ent );
			it = entity_list().objects.erase(it);
			delete ent;
			continue;
		}

		++it;
	}
}

void entity_update_physics()
{
	for(auto& entity : entity_list().objects)
	{
		entity->set_current_transform_from_physics();
	}
}

void entity_update(float alpha)
{
	for(auto& entity : entity_list().objects)
	{
		if ( !(entity->flags & Entity::EF_DELETE_INSTANCE) )
		{
			entity->update(alpha);
		}
	}
	entity_deferred_delete( true );
}


void entity_shutdown()
{
	LOGV("entity_shutdown!\n");

	entity_list().purge();
}


Entity::Entity() :
	flags(0),
	collision_object(0),
//	motion_interface(0),
	model_index(-1)
{
	this->id = entity_list().count();
	
	entity_list().add( this );
	engine::api::instance()->entities()->add(this);
	LOGV( "Entity() - %p, %ld\n", this, (unsigned long)this->id );

} // Entity

Entity::~Entity()
{
	LOGV( "~Entity() - %p, %ld\n", this, (unsigned long)this->id );
	entity_list().remove( this );
	engine::api::instance()->entities()->remove(this);
	
	if (this->collision_object)
	{
		engine::api::instance()->physics()->destroy_object(this->collision_object);
		this->collision_object = 0;
	}

} // ~Entity


int32_t Entity::get_model_index() const
{
	return model_index;
}

uint32_t Entity::get_render_flags() const
{
	return RENDER_NONE;
}

void Entity::get_world_transform(glm::vec3& out_position, glm::quat& out_orientation) const
{
	out_position = position;
	out_orientation = orientation;
}

//void Entity::set_transform(const gemini::EntityTransform& txform)
//{
//	if (this->collision_object)
//	{
//		collision_object->set_world_position(txform.local_position);
//	}
//}

void Entity::pre_tick()
{
	
}

void Entity::post_tick()
{
	set_current_transform_from_physics();
}

void Entity::update(float alpha)
{
	
} // update

void Entity::remove()
{
	this->flags = EF_DELETE_INSTANCE;
}

void Entity::collision_began(Entity* other)
{
}

void Entity::collision_ended(Entity* other)
{
}

void Entity::use(Entity *user)
{
}

void Entity::set_physics_from_current_transform()
{
	if (collision_object)
	{
		collision_object->set_world_transform(position, orientation);
	}
}

void Entity::set_current_transform_from_physics()
{
	if (this->collision_object)
	{
		collision_object->get_world_transform(position, orientation);
	}
}

//
// MEMORY OVERLOADS
//
void* Entity::operator new(size_t bytes)
{
	return engine::api::instance()->allocate(bytes);
}

void Entity::operator delete(void* memory)
{
	engine::api::instance()->deallocate(memory);
}

//
// PHYSICS
//

ICollisionObject* Entity::physics_create_static()
{
	assert(engine::api::instance() && engine::api::instance()->physics());
	
	physics::ObjectProperties properties;
	return engine::api::instance()->physics()->create_physics_model(model_index, properties);
}





ICollisionObject* Entity::physics_create_model()
{
	assert(engine::api::instance() && engine::api::instance()->physics());
	
	// If you hit this assert, you need to set_model on this Entity before
	// attempting to create a physics object from it.
	assert(model_index != -1);
	
	physics::ObjectProperties properties;
	properties.mass_kg = 10.0f;
	properties.restitution = 0.5f;
	properties.friction = 0.5f;
	return engine::api::instance()->physics()->create_physics_model(model_index, properties);
}

void Entity::set_physics_object(physics::ICollisionObject *object)
{
	assert(object != 0);
	object->set_user_data(this);
	this->collision_object = object;
	this->collision_object->set_collision_callback(entity_collision_callback);
}

void Entity::set_position(glm::vec3 *new_position)
{
	position = *new_position;
	
	if (collision_object)
	{
		collision_object->set_world_transform(*new_position, orientation);
	}
}


void Entity::apply_force(glm::vec3* force, glm::vec3* local_position)
{
//	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
//	{
//		this->collision_object->apply_force(*force, *local_position);
//	}
}

void Entity::apply_central_force(glm::vec3 *force)
{
//	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
//	{
//		this->collision_object->apply_central_force(*force);
//	}
}

void Entity::set_mass(float mass)
{
//	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
//	{
//		this->collision_object->set_mass(mass);
//	}
}

void Entity::set_parent(Entity *other)
{
//	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
//	{
//		this->collision_object->set_parent(this->collision_object, other->collision_object);
//	}
}

glm::vec3* Entity::get_position()
{
	return &position;
}

void Entity::set_model(const char* path)
{
//	engine::instance()->models()->destroy_instance_data(model_index);
	model_index = engine::api::instance()->models()->create_instance_data(path);
//	LOGV("set model index: %i, for model: %s\n", model_index, path);
}

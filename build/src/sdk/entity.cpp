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
#include <platform/mem.h>

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

void entity_physics_update(float delta_seconds)
{
	// step entities
	EntityListType::Collection::iterator it = entity_list().objects.begin();
	EntityListType::Collection::iterator end = entity_list().objects.end();
	for( ; it != end; ++it )
	{
		(*it)->fixed_update(delta_seconds);
	}
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

void entity_update()
{
	for(auto& entity : entity_list().objects)
	{
		if ( !(entity->flags & Entity::EF_DELETE_INSTANCE) )
		{
			entity->update();
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


void Entity::fixed_update( float delta_seconds )
{
} // fixed_update

void Entity::update()
{
	set_current_transform_from_physics();
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

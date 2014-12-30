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


#include "entity_manager.h"
#include <sdk/engine_api.h>
#include <sdk/physics_api.h>
#include <sdk/model_api.h>




using namespace gemini;
using namespace gemini::physics;

static void entity_collision_callback(CollisionEventType type, CollisionObject* first, CollisionObject* second)
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
			ent1->collision_began(ent0);
		}
		else if (type == Collision_Ended)
		{
			ent0->collision_ended(ent1);
			ent1->collision_ended(ent0);
		}
	}
}


//void EntityMotionInterface::get_transform(glm::vec3& position, const glm::quat& orientation)
//{
//			
//}
//
//void EntityMotionInterface::set_transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& mass_center_offset)
//{
//	this->node->local_position = mass_center_offset;
//	this->node->translation = position + mass_center_offset;
//	this->node->rotation = orientation;
//	this->target->position = position;
//}



void entity_startup()
{
}

void entity_post_script_load()
{
}


void entity_prestep()
{
	// this updates the ent's position with that of the physics body
	EntityListType::Collection::iterator it = entity_list().objects.begin();
	EntityListType::Collection::iterator end = entity_list().objects.end();
	Entity* ent;
	for( ; it != end; ++it )
	{
		ent = (*it);
		if (ent->collision_object)
		{
			// copy body's position over to node and entity
			const glm::vec3& world_position = ent->collision_object->get_world_position();
//			ent->node->translation = world_position;
			ent->position = world_position;
		}
	}
}

void entity_step()
{
	// step entities
	EntityListType::Collection::iterator it = entity_list().objects.begin();
	EntityListType::Collection::iterator end = entity_list().objects.end();
	for( ; it != end; ++it )
	{
//		(*it)->fixed_update( kernel::instance()->parameters().step_interval_seconds );
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
			it = entity_list().objects.erase( it );
//			ent->flags &= ~Entity::EF_DELETE_INSTANCE;
			delete ent;
			continue;
		}

		++it;
	}
}

void entity_tick()
{
	// tick entities
	EntityListType::Collection::iterator it = entity_list().objects.begin();
	EntityListType::Collection::iterator end = entity_list().objects.end();
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


void entity_shutdown()
{
	LOGV("entity_shutdown!\n");

	entity_list().purge();
}


Entity::Entity() :
	flags(0),
	collision_object(0),
//	motion_interface(0),
	model_index(0)
{
	this->id = entity_list().count();
	
	entity_list().add( this );
	LOGV( "Entity() - %p, %ld\n", this, (unsigned long)this->id );

} // Entity

Entity::~Entity()
{
	LOGV( "~Entity() - %p, %ld\n", this, (unsigned long)this->id );
//	entity_list().remove( this );
	
	if (this->collision_object)
	{
		DESTROY(CollisionObject, this->collision_object);
	}
	
//	if (this->motion_interface)
//	{
//		DESTROY(EntityMotionInterface, this->motion_interface);
//	}
} // ~Entity

//void Entity::set_model_index(int32_t index)
//{
//	model_index = index;
//}

int32_t Entity::get_model_index() const
{
	return model_index;
}

//gemini::EntityTransform Entity::get_transform() const
//{
//	gemini::EntityTransform txform;
//	return txform;
//}
//
//void Entity::set_transform(const gemini::EntityTransform& txform)
//{
//	
//}


void Entity::fixed_update( float delta_seconds )
{
} // fixed_update

void Entity::update()
{
} // update

void Entity::remove()
{
	this->flags = 1;
}

void Entity::collision_began(Entity* other)
{
}

void Entity::collision_ended(Entity* other)
{
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

CollisionObject* Entity::physics_create_static()
{
	assert(engine::api::instance() && engine::api::instance()->physics());
	
	return engine::api::instance()->physics()->create_physics_model(model_index, 0.0f, 0, glm::vec3(0, 0, 0));
}



void Entity::set_position(glm::vec3 *new_position)
{
	position = *new_position;
	
	if (collision_object)
	{
		collision_object->set_world_position(*new_position);
	}
}


void Entity::apply_force(glm::vec3* force, glm::vec3* local_position)
{
	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
	{
		this->collision_object->apply_force(*force, *local_position);
	}
}

void Entity::apply_central_force(glm::vec3 *force)
{
	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
	{
		this->collision_object->apply_central_force(*force);
	}
}

void Entity::set_mass(float mass)
{
	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
	{
		this->collision_object->set_mass(mass);
	}
}

void Entity::set_parent(Entity *other)
{
	if (this->collision_object && this->collision_object->is_type(CollisionType_Dynamic))
	{
		this->collision_object->set_parent(this->collision_object, other->collision_object);
	}
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

void Entity::set_physics(int physics_type)
{
	if (this->collision_object)
	{
		LOGW("Physics type already set on this entity! ignoring\n");
		return;
	}
	
	float mass_kg = 0.0f;
	
	if (physics_type == 0)
	{
		this->collision_object = physics_create_static();
	}
	else if (physics_type == 1)
	{
		// set dynamic physics type
		mass_kg = 5.0f;
	}
	else if (physics_type == 2)
	{
		// character
//		KinematicCharacter* controller = get_character_controller(0);
//		assert(controller);
//		this->collision_object = create_character_proxy(controller);
	}
	else if (physics_type == 3)
	{
		// ghost/trigger
//		this->collision_object = create_trigger(glm::vec3(1, 1, 1));
	}
	else
	{
		// If you reach this, the physics type is unknown/unsupported!
		assert(0);
	}

//	this->motion_interface = CREATE(EntityMotionInterface, this, this->node);
	
	// generate physics body from mesh
	// until we need a different system.
//	if (mesh)
//	{
//		this->collision_object = physics::create_physics_for_mesh(mesh, mass_kg, this->motion_interface, mesh->mass_center_offset);
//
//		if (this->collision_object)
//		{
//			this->collision_object->set_world_position(position);
//		}
//		else
//		{
//			LOGW("Unable to create physics body!\n");
//		}
//	}


	
	if (this->collision_object)
	{
		this->collision_object->set_collision_callback(entity_collision_callback);
		this->collision_object->set_user_data(this);
	}
}

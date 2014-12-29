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
#include "physics.h"
#include "camera.h"
#include "kernel.h"
#include "physics_rigidbody.h"

#include "charactercontroller.h"

#include "scene_graph.h"
#include "assets/asset_mesh.h"


#include "entity_manager.h"
#include "game_interface.h"


#include "engine_interface.h"

static gemini::game::GameInterface gamefuncs;

static void entity_collision_callback(physics::CollisionEventType type, physics::CollisionObject* first, physics::CollisionObject* second)
{
	assert(first != 0);
	assert(second != 0);
	
	Entity* ent0 = static_cast<Entity*>(first->get_user_data());
	Entity* ent1 = static_cast<Entity*>(second->get_user_data());
	
	if (ent0 && ent1)
	{
		if (type == physics::Collision_Began)
		{
			ent0->collision_began(ent1);
			ent1->collision_began(ent0);
		}
		else if (type == physics::Collision_Ended)
		{
			ent0->collision_ended(ent1);
			ent1->collision_ended(ent0);
		}
	}
}


void EntityMotionInterface::get_transform(glm::vec3& position, const glm::quat& orientation)
{
			
}

void EntityMotionInterface::set_transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& mass_center_offset)
{
	this->node->local_position = mass_center_offset;
	this->node->translation = position + mass_center_offset;
	this->node->rotation = orientation;
	this->target->position = position;
}



void entity_startup(const gemini::game::GameInterface& giface)
{
	gamefuncs = giface;
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
			ent->node->translation = world_position;
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
		(*it)->fixed_update( kernel::instance()->parameters().step_interval_seconds );
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


Entity::Entity() :
	flags(0),
	collision_object(0),
	node(0),
	motion_interface(0),
	model_index(0)
{
	this->id = entity_list().count();
	
	entity_list().add( this );
	LOGV( "Entity() - %p, %ld\n", this, (unsigned long)this->id );

} // Entity

Entity::~Entity()
{
	LOGV( "~Entity() - %p, %ld\n", this, (unsigned long)this->id );
	entity_list().remove( this );
	
	if (this->collision_object)
	{
		DESTROY(CollisionObject, this->collision_object);
	}
	
	if (this->motion_interface)
	{
		DESTROY(EntityMotionInterface, this->motion_interface);
	}

	if (this->node)
	{
		// remove the node
		if (this->node->parent)
		{
			this->node->parent->remove_child(this->node);
		}
		DESTROY(Node, this->node);
		this->node = 0;
	}
} // ~Entity

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

void Entity::set_position(glm::vec3 *new_position)
{
	position = *new_position;
	
	if (collision_object)
	{
		collision_object->set_world_position(*new_position);
	}

	// if this is a static body; we have to sync the position
	// with the node because there is no 'motion state' to do it automatically.
	if (this->node)
	{
		this->node->translation = position;
	}
}


void Entity::apply_force(glm::vec3* force, glm::vec3* local_position)
{
	if (this->collision_object && this->collision_object->is_type(physics::CollisionType_Dynamic))
	{
		this->collision_object->apply_force(*force, *local_position);
	}
}

void Entity::apply_central_force(glm::vec3 *force)
{
	if (this->collision_object && this->collision_object->is_type(physics::CollisionType_Dynamic))
	{
		this->collision_object->apply_central_force(*force);
	}
}

void Entity::set_mass(float mass)
{
	if (this->collision_object && this->collision_object->is_type(physics::CollisionType_Dynamic))
	{
		this->collision_object->set_mass(mass);
	}
}

void Entity::set_parent(Entity *other)
{
	if (this->collision_object && this->collision_object->is_type(physics::CollisionType_Dynamic))
	{
		this->collision_object->set_parent(this->collision_object, other->collision_object);
	}
}

glm::vec3* Entity::get_position()
{
	return &position;
}

assets::Mesh* load_mesh(scenegraph::Node* root, const char* path, scenegraph::Node*& node)
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
	
	return mesh;
}

void Entity::set_model(const char* path)
{
	//mesh = load_mesh(get_entity_root(), path, this->node);
	
	model_index = engine::instance()->load_model(path);
	
//	engine::instance()->set_model_index(this->node, model_index);
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
	}
	else if (physics_type == 1)
	{
		// set dynamic physics type
		mass_kg = 5.0f;
	}
	else if (physics_type == 2)
	{
		// character
		physics::KinematicCharacter* controller = physics::get_character_controller(0);
		assert(controller);
		this->collision_object = physics::create_character_proxy(controller);
	}
	else if (physics_type == 3)
	{
		// ghost/trigger
		this->collision_object = physics::create_trigger(glm::vec3(1, 1, 1));
	}
	else
	{
		// If you reach this, the physics type is unknown/unsupported!
		assert(0);
	}
		
	this->motion_interface = CREATE(EntityMotionInterface, this, this->node);
	
	// generate physics body from mesh
	// until we need a different system.
	if (model_index > 0)
	{
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
	}
	
	if (this->collision_object)
	{
		this->collision_object->set_collision_callback(entity_collision_callback);
		this->collision_object->set_user_data(this);
	}
}

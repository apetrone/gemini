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
#pragma once

#include <vector>

#include <platform/mem.h>

#include <core/str.h>
#include <core/stackstring.h>
//#include "physics/physics.h"

#include <sdk/iengineentity.h>
#include <sdk/utils.h>

#define DECLARE_ENTITY(entity_class, base_class)\
	typedef entity_class ThisClass;\
	typedef base_class BaseClass

#define DECLARE_ENTITY_NO_BASE(entity_class)\
	typedef entity_class ThisClass;\
	typedef entity_class BaseClass;

#define LINK_ENTITY_TO_CLASS(entity_class, classname)\
	static EntityFactoryClass<entity_class> classname(#classname)


//typedef std::vector< struct Entity*, GeminiAllocator<struct Entity*> > EntityVector;

class Entity;

namespace gemini
{
	class EntityManager;
	class GameInterface;
	
	namespace physics
	{
		class ICollisionObject;
	}
//	class EntityMotionInterface : public physics::PhysicsMotionInterface
//	{
//		Entity* target;
//		scenegraph::Node* node;
//		
//	public:
//		
//		EntityMotionInterface(Entity* entity, scenegraph::Node* sgnode) : target(entity), node(sgnode)
//		{
//		}
//		
//		virtual void get_transform(glm::vec3& position, const glm::quat& orientation);
//		
//		virtual void set_transform(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& mass_center_offset);
//	};
	
}


typedef gemini::StackString<128> EntityName;


class Entity : public gemini::IEngineEntity
{
public:
	DECLARE_ENTITY_NO_BASE(Entity);
	
	enum Flags
	{
		EF_NONE = 0,
		EF_DELETE_INSTANCE,	// remove this instance
	};

	uint64_t id;
	uint32_t flags;
	int32_t model_index;
	EntityName name;

	glm::vec3 position;
	glm::quat orientation;
	
	Entity();
	virtual ~Entity();
	
//	virtual void set_model_index(int32_t index);
	virtual int32_t get_model_index() const;
	
	virtual uint32_t get_render_flags() const;
	
	virtual void get_world_transform(glm::vec3& position, glm::quat& orientation) const;
	virtual void get_render_position(glm::vec3& out_position) const { out_position = position; }

	// called after the constructor
	virtual void spawn() {};
	
	// called after each entity has spawned
	virtual void activate() {};
	
	virtual void fixed_update(float delta_seconds);
	virtual void update();
	
	virtual void remove();
	
	virtual void collision_began(Entity* other);
	virtual void collision_ended(Entity* other);
	
	// Use is called on this entity
	virtual void use(Entity* user);
	
	virtual bool is_player() const { return false; }
	
	// Call this when you need to explicitly set the physics transform
	// from the current entity's position and rotation. Normally, you don't
	// need to use this; but the player controller makes use of it.
	void set_physics_from_current_transform();
	
	// Set the entity's transform (position/orientation) from the associated
	// physics object, if one is set.
	void set_current_transform_from_physics();
	
public:
	// memory overloads
	void* operator new(size_t bytes);
	void operator delete(void* memory);
	
	
public:
	// ACCESSORS
	void set_position(glm::vec3* new_position);
	glm::vec3* get_position();
	
public:
	
	//
	// PHYSICS
	//

	// create a static physics object using the model's collision model
	// TODO: also expose a way to create a static box?
	gemini::physics::ICollisionObject* physics_create_static();
	
//	physics::ICollisionObject* physics_create_sphere(const glm::vec3& local_center, float radius);
	
	// create a collision box to use for physics
//	physics::ICollisionObject* physics_create_box(const glm::vec3& local_center, const glm::vec3& mins, const glm::vec3& maxs);
	
	// use the collision model associated with this entity's model to create
	// a physics body.
	gemini::physics::ICollisionObject* physics_create_model();
	
	// create a trigger
//	physics::ICollisionObject* physics_create_trigger(const glm::vec3& local_center, const glm::vec3& mins, const glm::vec3& maxs);
	
	
	void set_physics_object(gemini::physics::ICollisionObject* object);
	
	
	
	
	// get/set functions for script interop
	const EntityName& get_name() { return this->name; }
	void set_name( const EntityName& object_name ) { this->name = object_name; }



	void apply_force(glm::vec3* force, glm::vec3* local_position);
	void apply_central_force(glm::vec3* force);
	void set_mass(float mass);
	void set_parent(Entity* other);
	
	// perhaps move these into a unified interface?
	gemini::physics::ICollisionObject* collision_object;
		
	// functions for this script object
	void set_model(const char* path);
}; // Entity


void entity_startup();
void entity_post_script_load();
void entity_shutdown();
void entity_physics_update(float delta_seconds);
void entity_update();
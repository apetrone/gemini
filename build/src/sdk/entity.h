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

#include <platform/mem.h>

#include <core/str.h>

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

struct Entity;

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





struct Entity : public gemini::IEngineEntity
{
	DECLARE_ENTITY_NO_BASE(Entity);
	
	enum Flags
	{
		EF_NONE = 0,
		EF_DELETE_INSTANCE,	// remove this instance
	};

	uint64_t id;
	uint32_t flags;
	int32_t model_index;
	String name;
	glm::vec3 position;
	glm::quat orientation;
	
	Entity();
	virtual ~Entity();
	
//	virtual void set_model_index(int32_t index);
	virtual int32_t get_model_index() const;
	
	virtual uint32_t get_render_flags() const;
	
	virtual void get_world_transform(glm::vec3& position, glm::quat& orientation) const;
	
	// called after the constructor
	virtual void spawn() {};
	
	// called after each entity has spawned
	virtual void activate() {};
	
	virtual void fixed_update(float delta_seconds);
	virtual void update();
	
	virtual void remove();
	
	virtual void collision_began(Entity* other);
	virtual void collision_ended(Entity* other);
	
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
	const String & get_name() { return this->name; }
	void set_name( const String & object_name ) { this->name = object_name; }



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
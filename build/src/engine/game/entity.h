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

#include <core/str.h>

#include "physics.h"

//typedef std::vector< struct Entity*, GeminiAllocator<struct Entity*> > EntityVector;

struct Entity;

namespace assets
{
	class Mesh;
}

namespace scenegraph
{
	struct Node;
}

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


struct Entity
{
	enum Flags
	{
		EF_NONE = 0,
		EF_DELETE_INSTANCE,	// remove this instance
	};

	uint64_t id;
	uint32_t flags;
	
	String name;
	
	Entity();
	virtual ~Entity();
	EntityMotionInterface* motion_interface;
	
	virtual void fixed_update(float delta_seconds);
	virtual void update();
	
	virtual void remove();
	
	virtual void collision_began(Entity* other);
	virtual void collision_ended(Entity* other);
	
	// get/set functions for script interop
	const String & get_name() { return this->name; }
	void set_name( const String & object_name ) { this->name = object_name; }
	

	
	glm::vec3 position;
	void set_position(glm::vec3* new_position);
	glm::vec3* get_position();

	void apply_force(glm::vec3* force, glm::vec3* local_position);
	void apply_central_force(glm::vec3* force);
	void set_mass(float mass);
	void set_parent(Entity* other);
	
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



void entity_startup();
void entity_post_script_load();
void entity_shutdown();
void entity_step();
void entity_tick();
void entity_set_scene_root(scenegraph::Node* root);
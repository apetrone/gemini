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
#pragma once
#include "memory.h"
#include <vector>
#include "factory.h"
#include "renderstream.h"
#include "camera.h"

enum ComponentTypes
{
	CMovement,
	CGraphics,
	CPhysics,
	CParticleEmitter,
	
	CMaxComponents
};

enum ComponentType
{
	MovementComponent,			// movement (position+velocity)
	InputMovementComponent,
	SpriteComponent,			// sprite component
	ParticleEmitterComponent,	// particles
	PhysicsComponent,			// physics-based component

	MaxComponentTypes
};

enum ComponentFlags
{
	C_INVALID = 0,
	C_DISABLE = 1,
};

class IComponent
{
public:
	unsigned int reference_id;
	unsigned short component_flags;

	IComponent() : component_flags(0) {}
	virtual ~IComponent() {}
	virtual ComponentType component_type() const = 0;
	
	unsigned int ref_id() const { return reference_id; }
	
	
	void set_flags(unsigned short flags) { component_flags = flags; }

}; // IComponent

typedef void (*ComponentCallback)( IComponent *, void * data );

class GenericComponentContainer
{
public:
	virtual ~GenericComponentContainer() {}
	
	virtual IComponent * create() = 0;
	virtual void destroy( IComponent * component ) = 0;
	virtual void for_each( ComponentCallback callback, void * data ) = 0;
	virtual IComponent * find_id( unsigned int reference_id ) = 0;
	virtual void step( float delta_seconds ) = 0;
	virtual void tick( float delta_seconds, float step_alpha ) = 0;
	virtual void purge() = 0;
};

struct Color;
namespace renderer
{
	struct VertexStream;
}


namespace assets
{
	class Material;
};

class ParticleEmitter;
struct RenderControl
{
	Camera camera;
	RenderStream rs;
	renderer::VertexStream * stream;
	unsigned int attribs;
	
	glm::mat4 modelview;
	glm::mat4 projection;
	
	void add_sprite_to_layer( unsigned short layer, int x, int y, int width, int height, const Color & color, float * texcoords );
	void render_stream( assets::Material * material );
	void render_emitter( ParticleEmitter * emitter );
};


namespace ComponentManager
{
	typedef std::vector<IComponent*> ComponentVector;
	typedef Factory<IComponent, MaxComponentTypes> ComponentFactory;

	
	
//	void register_component( const char * component_name, ComponentFactory::TypeCreator );
//	IComponent * create_component( const char * component_name );
	IComponent * create_type( ComponentType type );
	void destroy_type( IComponent * component );
	void purge();
	void step( float delta_seconds );
	void tick( float delta_seconds, float step_alpha );
	void draw( RenderControl & render_control );
//	ComponentVector & component_list( ComponentType type );
	
	

	void for_each_component(ComponentType type, ComponentCallback callback, void * data);
	IComponent * component_matching_id( unsigned int id, ComponentType type );
	
	
	
	void register_components();
}; // ComponentManager




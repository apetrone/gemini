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
#include "memory.hpp"
#include <vector>
#include "factory.hpp"
#include "renderstream.hpp"
#include "camera.hpp"

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

	typedef void (*ComponentCallback)( IComponent *, void * data );
	
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




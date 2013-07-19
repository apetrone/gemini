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

enum ComponentType
{
	PositionComponent,			// position
	VelocityComponent,			// velocity
	MoveableComponent,			// moveable
	PhysicsComponent,			// physics-based component
	RenderComponent,			// meshes / sprites?
	ParticleSystemComponent,	// particles
//	EffectsComponent,			// full-screen effects

	
	MaxComponentTypes
};

class IComponent
{
public:
	virtual ~IComponent() {}
	virtual ComponentType component_type() const = 0;
	
	virtual void step( float dt_seconds ) = 0;
	virtual void tick( float step_alpha ) = 0;
}; // IComponent


namespace ComponentManager
{
	typedef std::vector<IComponent*> ComponentVector;
	typedef Factory<IComponent, MaxComponentTypes> ComponentFactory;

	
	void register_component( const char * component_name, ComponentFactory::TypeCreator );
	IComponent * create_component( const char * component_name );
	void purge();
	void step( float delta_seconds );
	void tick( float step_alpha );
	ComponentVector & component_list( ComponentType type );
}; // ComponentManager




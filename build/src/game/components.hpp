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
#include "componentmanager.hpp"
#include "render_utilities.hpp"
#include "renderer.hpp"
#include "vertexstream.hpp"

class Movement : public virtual IComponent
{
public:
	virtual ComponentType component_type() const { return MovementComponent; }
	
	render_utilities::PhysicsState<glm::vec2> position;
	glm::vec2 velocity;
	
	virtual void step( float dt_sec );
	virtual void tick( float step_alpha );
}; // Movement

#if 0
class Renderable : public virtual IComponent
{
public:
	virtual ComponentType component_type() const { return RenderComponent; }

	virtual void render( renderer::IRenderDriver * driver ) = 0;
}; // Renderable
#endif

#include <string>
#include "assets.hpp"

class Sprite : public virtual IComponent
{
public:
	// these compose the 'animation state'
	unsigned short current_animation;	// currently active animation
	unsigned short current_frame;		// current frame of the animation
	float animation_time;				// current time of the animation

	// this is the 'stateless' part of the animation that we reference
	assets::SpriteConfig * sprite_config;
	
	unsigned int material_id;
	unsigned short width;
	unsigned short height;
	short hotspot_x;
	short hotspot_y;
	
	unsigned short layer;
	
	Color color;
	glm::vec2 scale;

	float rotation;
	
	Sprite();
	virtual ComponentType component_type() const { return SpriteComponent; }
	virtual void render( RenderControl & render_control );
	virtual void step( float delta_seconds );
	virtual void tick( float step_alpha );

	void play_animation( const std::string & name );
	void load_from_spriteconfig( assets::SpriteConfig * config );
}; // Sprite

class ParticleEmitter;
class Emitter : public virtual IComponent
{
public:
	ParticleEmitter * emitter;
	
	Emitter();
	~Emitter();
	virtual ComponentType component_type() const { return ParticleEmitterComponent; }
	
	virtual void render( RenderControl & render_control );
	virtual void step( float delta_seconds );
	virtual void tick( float step_alpha );
}; // Emitter

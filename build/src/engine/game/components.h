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
#include "componentmanager.h"
#include "render_utilities.h"
#include "renderer.h"
#include "vertexstream.h"

class Movement : public IComponent
{
public:
	DECLARE_FACTORY_CLASS(Movement, IComponent);
	virtual ComponentType component_type() const { return MovementComponent; }

	render_utilities::PhysicsState<glm::vec2> position;
	glm::vec2 velocity;

	virtual void step( float dt_sec );
	virtual void tick( float delta_seconds, float step_alpha );
}; // Movement

class InputMovement : public IComponent
{
public:
	DECLARE_FACTORY_CLASS(InputMovement, IComponent);
	virtual ComponentType component_type() const { return InputMovementComponent; }

	render_utilities::PhysicsState<glm::vec2> position;
	glm::vec2 velocity;

	virtual void step( float dt_sec );
	virtual void tick( float delta_seconds, float step_alpha );
}; // InputMovement

#if 0
class Renderable : public IComponent
{
public:
	virtual ComponentType component_type() const { return RenderComponent; }

	virtual void render( renderer::IRenderDriver * driver ) = 0;
}; // Renderable
#endif

#include <string>
#include "assets.h"


class Sprite : public IComponent
{
public:
	DECLARE_FACTORY_CLASS(Sprite, IComponent);

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
	virtual void tick( float delta_seconds, float step_alpha );

	void play_animation( const std::string & name );
	void load_from_spriteconfig( assets::SpriteConfig * config );
	void constrain_to_screen();
}; // Sprite

class ParticleEmitter;
class Emitter : public IComponent
{
public:
	DECLARE_FACTORY_CLASS(Emitter, IComponent);

	ParticleEmitter * emitter;

	Emitter();
	~Emitter();
	virtual ComponentType component_type() const { return ParticleEmitterComponent; }

	virtual void render( RenderControl & render_control );
	virtual void step( float delta_seconds );
	virtual void tick( float delta_seconds, float step_alpha );
}; // Emitter


typedef void (*on_collision)(class AABB2Collision* self, class AABB2Collision* other);

class AABB2Collision : public IComponent
{
public:
	DECLARE_FACTORY_CLASS(AABB2Collision, IComponent);

//	render_utilities::PhysicsState<glm::vec2> position;
//	glm::vec2 velocity;
	glm::vec2 box;
	unsigned short collision_mask;
	on_collision collision_handler;

	AABB2Collision();
	~AABB2Collision();
	virtual ComponentType component_type() const { return PhysicsComponent; }

	virtual bool collides_with( AABB2Collision * other ) const;
	virtual void step( float dt_sec );
	virtual void tick( float delta_seconds, float step_alpha );
	virtual void get_aabb( AABB2 & aabb ) const;
	virtual unsigned short get_collision_mask() const;
	virtual void get_rotation( float & radians ) const;
}; // AABB2Collision


class BreakOnHit : public AABB2Collision
{
public:
	virtual bool collides_with( AABB2Collision* other ) const;
};

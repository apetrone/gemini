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
#include "typedefs.h"
#include "log.h"
#include "particlesystem.hpp"
#include "util.hpp"

// -------------------------------------------------------------
// Particle
// -------------------------------------------------------------
Particle::Particle()
{
	color.set( 255, 255, 255, 255 );
	life_remaining = 0;
	life_total = 0;
	size = 1.0f;
	material_id = 0;
} // Particle


// -------------------------------------------------------------
// ParticleEmitter
// -------------------------------------------------------------
ParticleEmitter::ParticleEmitter()
{
	max_particles = 0;
	num_particles_alive = 0;
	material_id = 0;
	particle_list = 0;
	
	life_min = 0;
	life_max = 1;
	
	size_min = 0;
	size_max = 1;
} // ParticleEmitter

ParticleEmitter::~ParticleEmitter()
{
	purge();
} // ~ParticleEmitter

void ParticleEmitter::init(unsigned int total_particles)
{
	max_particles = total_particles;
	particle_list = CREATE_ARRAY(Particle, max_particles);
	
	Particle * p = 0;
	for( int i = 0; i < max_particles; ++i )
	{
		p = &particle_list[i];
		p->life_remaining = 0;
		p->color = Color( 0, 0, 0, 255 );
		p->velocity = glm::vec3( 0, 0, 0 );
//		p->position = world_position;
		p->position.snap( world_position.current );
		p->size = 1.0f;
		p->material_id = material_id;
	}
} // init


void ParticleEmitter::step(float delta_seconds)
{
	unsigned int pid = 0;
	Particle * p = 0;
	num_particles_alive = 0;
	float delta_msec = (delta_seconds * 1000.0f);

	world_position.step(delta_seconds);

	this->next_spawn -= delta_msec;
	int particles_to_spawn = 0;
	if ( this->next_spawn <= 0 )
	{
		this->next_spawn = this->spawn_delay_seconds;
		particles_to_spawn = this->spawn_rate;
		if ( particles_to_spawn > max_particles-num_particles_alive )
			particles_to_spawn = (max_particles-num_particles_alive);
	}
	
	
	for( pid = 0; pid < max_particles; ++pid )
	{
		p = &particle_list[pid];
		p->life_remaining -= delta_msec;
		if ( p->life_remaining > 0.1 )
		{
			float lifet = 1.0 - (p->life_remaining / p->life_total);
			p->position.step(delta_seconds);
			p->position.current += (p->velocity * delta_seconds);
//			p->color = color_channel.get_value( lifet );
			p->color = Color( 255, 255, 255, 64 );
			p->color.a = 255.0*alpha_channel.get_value( lifet );
			//			graph->RecordFloat( p->color.a, 0 );
			p->size = size_channel.get_value( lifet );
			++num_particles_alive;
		}
		else if ( particles_to_spawn > 0 )
		{
			p->life_total = p->life_remaining = util::random_range( life_min, life_max );
			p->velocity = glm::vec3( util::random_range(velocity_min[0], velocity_max[0]), util::random_range(velocity_min[1], velocity_max[1]), util::random_range(velocity_min[2], velocity_max[2]) );
			p->position.snap( world_position.render );
			p->color = color_channel.get_value(0);
			//p->color = Color( 255, 255, 255 );
			p->color.a = alpha_channel.get_value(0);
			p->size = size_channel.get_value(0);
			--particles_to_spawn;
			++num_particles_alive;
		}
	}
} // step

void ParticleEmitter::purge()
{
	DESTROY_ARRAY( Particle, particle_list, max_particles );
} // purge


#include "kernel.hpp"

// -------------------------------------------------------------
// ParticleSystem
// -------------------------------------------------------------

ParticleSystem::ParticleSystem()
{
	emitter_list = 0;
	num_active_emitters = 0;
} // ParticleSystem

ParticleSystem::~ParticleSystem()
{
	purge();
} // ~ParticleSystem

void ParticleSystem::purge()
{
	DESTROY_ARRAY(ParticleEmitter, emitter_list, num_active_emitters);
	emitter_list = 0;
} // purge

void ParticleSystem::step( float delta_seconds )
{
	for( int emitter_id = 0; emitter_id < num_active_emitters; ++emitter_id )
	{
		ParticleEmitter * emitter = &emitter_list[emitter_id];
		emitter->step( delta_seconds );
	}
} // step
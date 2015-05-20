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
#include <platform/typedefs.h>

#include "particlesystem.h"

namespace gemini
{
	// -------------------------------------------------------------
	// Particle
	// -------------------------------------------------------------
	Particle::Particle()
	{
		color.set(255, 255, 255, 255);
		life_remaining = 0;
		life_total = 0;
		size = 1.0f;
	} // Particle


	// -------------------------------------------------------------
	// ParticleEmitter
	// -------------------------------------------------------------
	ParticleEmitter::ParticleEmitter()
	{
		emitter_config = 0;
		num_particles_alive = 0;
		particle_list = 0;
		next_spawn = 0;
	} // ParticleEmitter

	ParticleEmitter::~ParticleEmitter()
	{
		purge();
	} // ~ParticleEmitter

	void ParticleEmitter::init()
	{
		particle_list = MEMORY_NEW_ARRAY(Particle, this->emitter_config->max_particles, platform::memory::global_allocator());

		this->next_spawn = 0;
		this->num_particles_alive = this->emitter_config->max_particles;
		
		Particle * p = 0;
		for(int i = 0; i < this->emitter_config->max_particles; ++i)
		{
			p = &particle_list[i];
			generate_particle( p );
			// force the particles to spawn next step: this is mainly done
			// to ensure that the world position is set properly before they're
			// rendered -- as it may not be set to the desired value at this point.
			p->life_total = p->life_remaining = 0;
		}
	} // init


	void ParticleEmitter::step(float delta_seconds)
	{
		unsigned int pid = 0;
		Particle * p = 0;
		
		float delta_msec = (delta_seconds * 1000.0f);
		
		if (!emitter_config)
		{
			return;
		}
		
		world_position.step(delta_seconds);

		this->next_spawn -= delta_msec;
		uint32_t particles_to_spawn = 0;
		if (this->next_spawn <= 0)
		{
			this->next_spawn = this->emitter_config->spawn_delay_seconds;
			particles_to_spawn = this->emitter_config->spawn_rate;
			uint32_t eff = (this->emitter_config->max_particles-num_particles_alive);
			if (particles_to_spawn > eff )
			{
				particles_to_spawn = (this->emitter_config->max_particles-num_particles_alive);
	//			LOGV( "particle_to_spawn: %i\n", particles_to_spawn );
			}
		}
		
		num_particles_alive = 0;
		for(pid = 0; pid < this->emitter_config->max_particles; ++pid)
		{
			p = &particle_list[pid];
			p->life_remaining -= delta_msec;
			
			if (p->life_remaining > 0.1)
			{
				float lifet = 1.0 - (p->life_remaining / p->life_total);
				p->position.step(delta_seconds);
				p->position.current += (p->velocity * delta_seconds);
				p->color = this->emitter_config->color_channel.get_value(lifet);
				p->color.a = 255.0 * this->emitter_config->alpha_channel.get_value(lifet);
				p->size = this->emitter_config->size_channel.get_value(lifet);
				++num_particles_alive;
			}
			else if (particles_to_spawn > 0)
			{
				generate_particle( p );
				--particles_to_spawn;
				++num_particles_alive;
			}
		}
	} // step

	void ParticleEmitter::purge()
	{
		if (this->emitter_config && this->particle_list)
		{
			MEMORY_DELETE_ARRAY(particle_list, platform::memory::global_allocator());
		}
	} // purge


	void ParticleEmitter::load_from_emitter_config(assets::EmitterConfig* emitter_config)
	{
		if (emitter_config)
		{
			purge();
			
			this->emitter_config = emitter_config;
			
			init();
		}
	} // load_from_emitter_config

	void ParticleEmitter::generate_particle( Particle * p )
	{
		p->life_total = p->life_remaining = core::util::random_range(this->emitter_config->life.min, this->emitter_config->life.max);
		p->velocity = glm::vec3(core::util::random_range(
												   this->emitter_config->velocity.min[0], this->emitter_config->velocity.max[0]),
								core::util::random_range(this->emitter_config->velocity.min[1], this->emitter_config->velocity.max[1]),
								core::util::random_range(this->emitter_config->velocity.min[2], this->emitter_config->velocity.max[2]));
		
		p->position.snap(world_position.render);
	//	LOGV( "p->position: %g, %g %g\n", p->position.render.x, p->position.render.y, p->position.render.z );
		p->color = this->emitter_config->color_channel.get_value(0);
		p->color.a = 255.0 * this->emitter_config->alpha_channel.get_value(0);
		p->size = this->emitter_config->size_channel.get_value(0);
	} // generate_particle

	// -------------------------------------------------------------
	// ParticleSystem
	// -------------------------------------------------------------

	ParticleSystem::ParticleSystem()
	{
	} // ParticleSystem

	ParticleSystem::~ParticleSystem()
	{
		purge();
	} // ~ParticleSystem

	void ParticleSystem::purge()
	{
		ParticleEmitterVector::iterator iter = emitters.begin();
		for( ; iter != emitters.end(); ++iter)
		{
			ParticleEmitter* emitter = (*iter);
			MEMORY_DELETE(emitter, platform::memory::global_allocator());
		}
		
		emitters.clear();
	} // purge

	void ParticleSystem::step(float delta_seconds)
	{
		ParticleEmitterVector::iterator iter = emitters.begin();
		for( ; iter != emitters.end(); ++iter)
		{
			ParticleEmitter* emitter = (*iter);
			emitter->step( delta_seconds );
		}
	} // step

	ParticleEmitter* ParticleSystem::add_emitter()
	{
		ParticleEmitter* emitter = MEMORY_NEW(ParticleEmitter, platform::memory::global_allocator());
		this->emitters.push_back(emitter);
		return emitter;
	} // add_emitter

	void ParticleSystem::remove_emitter(ParticleEmitter* emitter)
	{
		ParticleEmitterVector::iterator iter = emitters.begin();
		
		for( ; iter != emitters.end(); ++iter)
		{
			if (emitter == (*iter))
			{
				MEMORY_DELETE(emitter, platform::memory::global_allocator());
				emitters.erase(iter);
				break;
			}
		}
	} // remove_emitter
} // namespace gemini
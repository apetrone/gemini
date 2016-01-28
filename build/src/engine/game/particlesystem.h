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

#include "keyframechannel.h"
#include "assets/asset_emitter.h"

#include <core/mem.h>
#include <core/util.h>
#include <core/mathlib.h>

#include <renderer/render_utilities.h>
#include <renderer/color.h>

#include <vector>

namespace gemini
{
	// -------------------------------------------------------------
	// Particle
	// -------------------------------------------------------------
	struct Particle
	{
		Color color;
		::renderer::render_utilities::PhysicsState<glm::vec3> position;
		glm::vec3 velocity;

		float life_remaining;
		float life_total;
		float size;

		Particle();
	//	~Particle()=default;
	}; // Particle





	// -------------------------------------------------------------
	// ParticleEmitter
	// -------------------------------------------------------------
	struct ParticleEmitter
	{
		enum
		{
			PLANE,
			SPHERE,
			CONE,
			BOX
		};

		assets::EmitterConfig * emitter_config;




		::renderer::render_utilities::PhysicsState<glm::vec3> world_position;
		unsigned int num_particles_alive;
		Particle * particle_list;
		float next_spawn;

		ParticleEmitter();
		~ParticleEmitter();
		void init();
		void step( float delta_seconds );
		void purge();
		void load_from_emitter_config( assets::EmitterConfig * emitter_config );
		void generate_particle( Particle * particle );
	}; // ParticleEmitter

	typedef std::vector<ParticleEmitter*> ParticleEmitterVector;

	// -------------------------------------------------------------
	// ParticleSystem
	// -------------------------------------------------------------
	struct ParticleSystem
	{
	//	unsigned int num_active_emitters;
	//	ParticleEmitter * emitter_list;
		ParticleEmitterVector emitters;

		ParticleSystem();
		~ParticleSystem();
		void purge();
		void step( float delta_seconds );

		ParticleEmitter * add_emitter();
		void remove_emitter( ParticleEmitter * emitter );
	}; // ParticleSystem
} // namespace gemini

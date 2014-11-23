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

#include <gemini/mem.h>
#include <gemini/util.h>

#include <renderer/color.h>
#include <gemini/mathlib.h>
#include "keyframechannel.h"
#include <renderer/render_utilities.h>
#include "assets/asset_emitter.h"

// -------------------------------------------------------------
// Particle
// -------------------------------------------------------------
struct Particle
{
	Color color;
	render_utilities::PhysicsState<glm::vec3> position;
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
	

	
	
	render_utilities::PhysicsState<glm::vec3> world_position;
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
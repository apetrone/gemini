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
#include "color.hpp"
#include "mathlib.h"
#include "keyframechannel.hpp"
#include "render_utilities.hpp"


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



template <class Type>
struct RangedValue
{
	Type min;
	Type max;
	
	void set_range( const Type & minimum, const Type & maximum )
	{
		this->min = minimum;
		this->max = maximum;
	}
}; // RangedValue

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

	render_utilities::PhysicsState<glm::vec3> world_position;
	
	unsigned int max_particles;
	unsigned int num_particles_alive;
	unsigned int material_id;
	Particle * particle_list;
	
	RangedValue<unsigned int> life;
	RangedValue<glm::vec3> velocity;
	RangedValue<float> size;
	
	// can this emitter emit particles?
	//	bool _canEmit;
	
	// can this emitter update its particles?
	//	bool _canUpdate;
	
	KeyframeChannel<Color> color_channel;
	KeyframeChannel<float> alpha_channel;
	KeyframeChannel<float> size_channel;
	
	float next_spawn;
	float spawn_delay_seconds;
	int spawn_rate;
	
	ParticleEmitter();
	~ParticleEmitter();
	void init( unsigned int max_particles );
	void step( float delta_seconds );
	void purge();
}; // ParticleEmitter

// -------------------------------------------------------------
// ParticleSystem
// -------------------------------------------------------------
struct ParticleSystem
{
	unsigned int num_active_emitters;
	ParticleEmitter * emitter_list;
	
	ParticleSystem();
	~ParticleSystem();
	void purge();
	void step( float delta_seconds );
}; // ParticleSystem
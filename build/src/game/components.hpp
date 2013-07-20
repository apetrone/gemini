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

	virtual ComponentType component_type() const { return SpriteComponent; }
//
//	struct Frame
//	{
//		renderer::UV texcoords[4];
//	}; // Frame
//	
//	struct Clip
//	{
//		std::string name;
//		unsigned short frame_start;
//		unsigned short total_frames;
//		Frame * frames;
//		
//		Clip();
//		~Clip();
//		
//		void create_frames( unsigned int material_id, unsigned int num_frames, unsigned int sprite_width, unsigned int sprite_height );
//		void purge_frames();
//		float * uvs_for_frame( unsigned short frame_id );
//		bool is_valid_frame(unsigned short frame_id);
//	}; // Clip
	
//	Clip * animations;					// animation frames
	unsigned short current_animation;	// currently active animation
	unsigned short current_frame;		// current frame of the animation
//	unsigned short total_animations;	// total animations
	float animation_time;				// current time of the animation
//	float frame_delay;					// delay in msec between each frame


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
	virtual void render( RenderControl & render_control );
	virtual void step( float delta_seconds );
	virtual void tick( float step_alpha );
	
//	
//	Clip * get_clip_by_index( unsigned short index );
//	
//	void create_animations( unsigned short num_animations );
//	void purge_animations();
	void play_animation( const std::string & name );
	
	
	void load_from_spriteconfig( assets::SpriteConfig * config );
}; // Sprite
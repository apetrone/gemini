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
#include "screencontrol.h"
#include "font.h"
#include "camera.h"
#include "renderstream.h"
#include "assets.h"
#include "audio.h"
#include "particlesystem.h"
//#include "map_event.h"
#include "renderer.h"
//#include "render_utilities.h"
#include "input.h"

struct RenameThisData
{
	int world_x;
	int world_y;
	renderer::UV uvs[4];
};


const float BULLET_SPEED = 500;
const float TEST_SIZE = 256;
const int STARTING_ENERGY = 10;

enum {
	GAME_PLAY = 1,
	GAME_WIN,
	GAME_FAIL
};



struct GameScreen : public IScreen
{
	assets::Font * font;
	assets::Font * round_title;
	renderer::VertexStream vb;
	assets::Shader default_shader;
	unsigned int test_attribs;

	assets::Material * player_mat;
	
	audio::SoundHandle player_fire, enemy_explode;
	audio::SoundSource player_source;
	short fire_delay;
	short next_fire;
	
	// scrolling layer
	unsigned int background_material_id;
	unsigned short background_num_columns;
	unsigned short background_tile_size;
	unsigned short background_num_rows;
	RenameThisData * background_layers;
	gemini::Recti cliprect;
	
	EventBasedMap event_based_map;
	unsigned int current_event;
	float current_gametime;
	

	
	ParticleSystem psys;
	
	GameScreen();
	~GameScreen();
	
	void render_layer( RenameThisData * layer );
	
	virtual void on_show( kernel::IApplication * app );
	virtual void on_hide( kernel::IApplication * app );
	virtual void on_draw( kernel::IApplication * app );
	virtual void on_update( kernel::IApplication * app );
	virtual void on_step( kernel::IApplication * app );
	
	virtual const char * name() const
	{
		return "GameScreen";
	}
	
	virtual void on_event( kernel::KeyboardEvent & event, kernel::IApplication * app );
	virtual void on_event( kernel::MouseEvent & event, kernel::IApplication * app ) {}
	virtual void on_event( kernel::TouchEvent & event, kernel::IApplication * app ) {}
}; // GameScreen

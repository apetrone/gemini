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
#include "screencontrol.hpp"
#include "font.hpp"
#include "camera.hpp"
#include "renderstream.hpp"
#include "assets.hpp"
#include "audio.hpp"
#include "particlesystem.hpp"
#include "map_event.hpp"
#include "renderer.hpp"
//#include "render_utilities.hpp"
#include "input.hpp"

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

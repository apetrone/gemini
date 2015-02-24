// -------------------------------------------------------------
// Copyright (C) 2013-, Adam Petrone
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

#include "kernel.h"

#include <platform/platform.h>

#include <stdio.h>
#include <core/logging.h>
#include <core/mathlib.h>
#include "debugdraw.h"
#include "input.h"
#include <renderer/renderer.h>
#include <renderer/renderstream.h>

#include "camera.h"

#include "assets/asset_mesh.h"
#include "assets/asset_material.h"

#include "btBulletDynamicsCommon.h"

#include "physics/physics.h"
#include "physics/physics_player_controller.h"

#include <renderer/font.h>
#include "assets/asset_font.h"

#include <core/mathlib.h>

#include "scenelink.h"
#include "vr.h"

#include <renderer/constantbuffer.h>

#include "audio.h"

#include <core/dictionary.h>

#include <core/filesystem.h>
#include <core/logging.h>
//#include <core/ringbuffer.h>

#include <sdk/audio_api.h>
#include <sdk/entity_api.h>
#include <sdk/iengineentity.h>
#include <sdk/model_api.h>
#include <sdk/engine_api.h>
#include <sdk/game_api.h>
#include <sdk/experimental_api.h>
#include <sdk/debugdraw_api.h>
#include <sdk/shared.h>

#include <platform/mem.h>

using namespace gemini;
using namespace gemini::physics;
using namespace core;

#define LOCK_CAMERA_TO_CHARACTER 1

// even if a VR device is attached, this will NOT render to it
// this allows debugging in some other mechanism to check sensor data.
uint8_t RENDER_TO_VR = 1;

uint8_t REQUIRE_RIFT = 1;

const uint16_t MAX_ENTITIES = 2048;


class IGameState
{
public:
	virtual ~IGameState() {}
	
	/// @desc Called when this game state becomes active
	virtual void activate() = 0;
	
	/// @desc Called when this game state should go inactive
	virtual void deactivate() = 0;
	
	/// @desc Called each frame
	/// @param framedelta_seconds The elapsed time since the last run_frame call (in seconds).
	virtual void run_frame(float framedelta_seconds) = 0;
};


class MainMenuState : public IGameState
{
public:
	virtual void activate() {}
	
	virtual void deactivate() {}
	
	virtual void run_frame(float framedelta_seconds) {}
};

class InGameMenuState : public IGameState
{
public:
	virtual void activate() {}
	
	virtual void deactivate() {}
	
	virtual void run_frame(float framedelta_seconds) {}
};

class GameState : public IGameState
{
public:
	virtual void activate() {}
	
	virtual void deactivate() {}
	
	virtual void run_frame(float framedelta_seconds) {}
};

class StateController
{
private:
	typedef Dictionary<IGameState*> StatesByHash;
	StatesByHash states;
	IGameState* active_game_state;

public:
	StateController() : active_game_state(0) {}
	virtual ~StateController() {}


	void add_state(const char* name, IGameState* state)
	{
		states.insert(name, state);
	}
	
	IGameState* state_by_name(const char* name)
	{
		IGameState* state = 0;
		states.get(name, state);
		return state;
	}
	
	IGameState* active_state() const { return active_game_state; }
};





class ProjectChimera : public kernel::IApplication,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>,
public kernel::IEventListener<kernel::GameControllerEvent>
{
public:
	DECLARE_APPLICATION(ProjectChimera);

	Camera* active_camera;

	vr::HeadMountedDevice* device;

	renderer::SceneLink scenelink;
	SceneRenderMethod* render_method;
	

	Camera main_camera;
	
	audio::SoundHandle background;
	audio::SoundSource background_source;
	
	bool draw_physics_debug;

	platform::DynamicLibrary* gamelib;
	disconnect_engine_fn disconnect_engine;
	
	EntityManager entity_manager;
	ModelInterface model_interface;
	Experimental experimental;
		
	IEngineInterface* engine_interface;
	IGameInterface* game_interface;

//	core::RingBuffer<UserCommand, 32> client_commands;

	bool has_focus;
	
	GUIRenderer gui_renderer;
	gui::Compositor* compositor;
	bool in_gui;
	gui::Graph* graph;
	
	audio::SoundHandle menu_show;
	audio::SoundHandle menu_hide;
	
	
	gui::Panel* root;
	gui::Button* newgame;
	gui::Button* test;
	gui::Button* quit;
	
	CustomListener gui_listener;
	
public:
	ProjectChimera() : gui_listener(in_gui)
	{
		device = 0;
		render_method = 0;
		active_camera = &main_camera;
		draw_physics_debug = true;

		game_interface = 0;

		has_focus = true;
		
		compositor = 0;
		in_gui = false;
		graph = 0;
	}
	
	virtual ~ProjectChimera()
	{
		DESTROY(IEngineInterface, engine_interface);
	}
	
	virtual void event( kernel::KeyboardEvent & event )
	{
		if (event.is_down)
		{
			if (event.key == input::KEY_ESCAPE)
			{
				in_gui = !in_gui;
				if (!in_gui)
				{
					center_mouse(kernel::parameters());
					audio::play(menu_hide);
				}
				else
				{
					audio::play(menu_show);
				}
				
				kernel::instance()->show_mouse(in_gui);
				
				root->set_visible(in_gui);
			}
			else if (event.key == input::KEY_SPACE)
			{
				if (device)
				{
					device->dismiss_warning();
				}
			}
			else if (event.key == input::KEY_TAB)
			{
				if (device)
				{
					device->reset_head_pose();
				}
			}
			else if (event.key == input::KEY_P)
			{
				draw_physics_debug = !draw_physics_debug;
				LOGV("draw_physics_debug = %s\n", draw_physics_debug?"ON":"OFF");
			}
			else if (event.key == input::KEY_M)
			{
				LOGV("level load\n");
				game_interface->level_load();
			}
		}
		
		
		if (in_gui && compositor)
		{
			compositor->key_event(event.key, event.is_down, 0);
		}
	}
	
	virtual void event(kernel::MouseEvent& event)
	{
		if (in_gui)
		{
			gui::CursorButton::Type input_to_gui[] = {
				gui::CursorButton::Left,
				gui::CursorButton::Right,
				gui::CursorButton::Middle,
				gui::CursorButton::Mouse4,
				gui::CursorButton::Mouse5
			};
			
			gui::CursorButton::Type button;
			
			
			switch( event.subtype )
			{
				case kernel::MouseMoved:
				{
					if ( compositor )
					{
						compositor->cursor_move_absolute( event.mx, event.my );
					}
					break;
				}
				case kernel::MouseButton:
					
					button = input_to_gui[ event.button ];
					if ( event.is_down )
					{
						fprintf( stdout, "mouse button %i is pressed\n", event.button );
					}
					else
					{
						fprintf( stdout, "mouse button %i is released\n", event.button );
					}
					
					if ( compositor )
					{
						compositor->cursor_button( button, event.is_down );
					}
					break;
					
				case kernel::MouseWheelMoved:
					if ( event.wheel_direction > 0 )
					{
						fprintf( stdout, "mouse wheel toward screen\n" );
					}
					else
					{
						fprintf( stdout, "mouse wheel away from screen\n" );
					}
					break;
				default:
					fprintf( stdout, "mouse event received!\n" );
					break;
			}
		}
	}

	virtual void event( kernel::SystemEvent & event )
	{
		if (event.subtype == kernel::WindowLostFocus)
		{
//			kernel::instance()->show_mouse(true);
			has_focus = false;
		}
		else if (event.subtype == kernel::WindowGainFocus)
		{
//			kernel::instance()->show_mouse(false);
			has_focus = true;
		}
	}
	
	virtual void event(kernel::GameControllerEvent& event)
	{
//		if (event.subtype == kernel::JoystickConnected)
//		{
//			LOGV("gamepad [%i] connected\n", event.gamepad_id);
//		}
//		else if (event.subtype == kernel::JoystickDisconnected)
//		{
//			LOGV("gamepad [%i] disconnected\n", event.gamepad_id);
//		}
//		else if (event.subtype == kernel::JoystickButton)
//		{
//			LOGV("gamepad [%i] button: %i, is_down: %i\n", event.gamepad_id, event.button, event.is_down);
//		}
//		else if (event.subtype == kernel::JoystickAxisMoved)
//		{
//			LOGV("gamepad [%i] joystick: %i, value: %i\n", event.gamepad_id, event.joystick_id, event.joystick_value);
//		}
//		else
//		{
//			LOGV("gamepad [%i] controller event received: %i\n", event.gamepad_id, event.subtype);
//		}
	}
	
	
	
	void test_animation()
	{
		
		Keyframe<float> key0;
		Keyframe<float> key1;
		key0.seconds = 0;
		key0.value = 0;
		key1.seconds = 20;
		key1.value = 1;
		
		KeyChannel<float> channel;
		channel.keys.allocate(2);
		channel.keys[0] = key0;
		channel.keys[1] = key1;
		channel.length_seconds = 20.0f;
		
		
		Keyframe<float> k0;
		Keyframe<float> k1;
		k0.seconds = 0.0f;
		k0.value = 0.0f;
		k1.seconds = 10.0f;
		k1.value = 100.0f;
				
		KeyChannel<float> channel2;
		channel2.keys.allocate(2);
		channel2.keys[0] = k0;
		channel2.keys[1] = k1;
		channel2.length_seconds = 10.0f;
		
		float local_value = -1.0f;
		
		
		
//		KeyframeChannelReference cr(channel, local_value);
		
		float time_seconds = 15.0f;
//		for (uint8_t step = 0; step < 10; ++step)
//		{
//			time_seconds += 0.1f;
//		}
		local_value = channel.value_at_time(time_seconds);
		float v2 = channel2.value_at_time(time_seconds);
		
		
		LOGV("local_value is: %2.4f, channel2.value is %2.4f\n", local_value, v2);
	}
	

	virtual kernel::ApplicationResult config( kernel::Parameters& params )
	{
		params.window_title = "project_chimera";
		params.window_width = 1280;
		params.window_height = 720;

		vr::startup();
		
		// if there's a rift connected
		if ((REQUIRE_RIFT && (vr::total_devices() > 0)) || (!REQUIRE_RIFT))
		{
			// create one
			device = vr::create_device();
			if (device)
			{
				int32_t width, height;
				device->query_display_resolution(width, height);

				
				// required such that the kernel doesn't swap buffers
				// as this is handled by the rift sdk.
				params.swap_buffers = !RENDER_TO_VR;
				
				if (RENDER_TO_VR)
				{
//					params.use_fullscreen = 1;

					
#if PLATFORM_MACOSX
					// target the rift.
					if (REQUIRE_RIFT)
					{
						params.target_display = 2;
					}
#endif
					params.use_vsync = true;
					params.window_width = (uint16_t)width;
					params.window_height = (uint16_t)height;
				}
			}
		}

		test_animation();
		
		return kernel::Application_Success;
	}

	void center_mouse(const kernel::Parameters& params)
	{
		if (has_focus && !in_gui)
		{
			kernel::instance()->warp_mouse(params.window_width/2, params.window_height/2);
		}
	}

	virtual kernel::ApplicationResult startup( kernel::Parameters& params )
	{
//		{
//			StateController sc;
//			
//			sc.add_state("mainmenu", new MainMenuState());
//			sc.add_state("ingame", new InGameMenuState());
//			sc.add_state("game", new GameState());
//			
//			
//			IGameState* mainmenu = sc.state_by_name("mainmenu");
//			mainmenu->activate();
//		}
	

		gui_listener.set_hover_sound(audio::create_sound("sounds/8b_select1"));
	
		menu_show = audio::create_sound("sounds/menu_show3");
		menu_hide = audio::create_sound("sounds/menu_hide");
	
		float camera_fov = 50.0f;
		if (device && RENDER_TO_VR)
		{
			vr::setup_rendering(device, params.render_width, params.render_height);
			render_method = CREATE(VRRenderMethod, device, scenelink);
		}
		else
		{
			render_method = CREATE(DefaultRenderMethod, scenelink);
		}
		
		engine_interface = CREATE(EngineInterface, &entity_manager, &model_interface, physics::api::instance(), &experimental, render_method, &main_camera);
		gemini::engine::api::set_instance(engine_interface);
		
//		background = audio::create_sound("sounds/8b_shoot");
//		background_source = audio::play(background, -1);

//		player_controller->camera = active_camera;
		active_camera->type = Camera::FIRST_PERSON;
		active_camera->move_speed = 0.1f;
		active_camera->perspective(camera_fov, params.render_width, params.render_height, 0.01f, 8192.0f);
		active_camera->set_absolute_position(glm::vec3(0, 4, 7));
		active_camera->pitch = 30;
		active_camera->update_view();
			
			
		compositor = new gui::Compositor(params.render_width, params.render_height);
		_compositor = compositor;
		compositor->set_renderer(&this->gui_renderer);
		compositor->set_listener(&gui_listener);
		
		root = new gui::Panel(compositor);
		root->set_bounds(0, 0, params.render_width, params.render_height);
		root->set_background_color(gui::Color(0, 0, 0, 192));
		root->set_visible(in_gui);
		compositor->add_child(root);
		
		gui::Color button_background(128, 128, 128, 255);
		gui::Color button_hover(255, 255, 128, 255);
		
		uint32_t button_width = 320;
		uint32_t button_height = 50;
		uint32_t button_spacing = 10;
		uint32_t total_buttons = 2;
		uint32_t vertical_offset = 0;
		uint32_t origin_x = (params.render_width/2.0f) - (button_width/2.0f);
		uint32_t origin_y = (params.render_height/2.0f) - ((button_height*total_buttons)/2.0f);
		
		
		
		newgame = new gui::Button(root);
		newgame->set_bounds(origin_x, origin_y, button_width, button_height);
		newgame->set_font(compositor, "fonts/default16");
		newgame->set_text("New Game");
		newgame->set_background_color(button_background);
		newgame->set_hover_color(button_hover);
		newgame->set_userdata((void*)2);
		root->add_child(newgame);
		origin_y += (button_height+button_spacing);
		
		test = new gui::Button(root);
		test->set_bounds(origin_x, origin_y, button_width, button_height);
		test->set_font(compositor, "fonts/default16");
		test->set_text("[test] render cubemap");
		test->set_background_color(button_background);
		test->set_hover_color(button_hover);
		test->set_userdata((void*)3);
		root->add_child(test);
		origin_y += (button_height+button_spacing);
		
		quit = new gui::Button(root);
		quit->set_bounds(origin_x, origin_y, button_width, button_height);
		quit->set_font(compositor, "fonts/default16");
		quit->set_text("Quit Game");
		quit->set_background_color(button_background);
		quit->set_hover_color(button_hover);
		quit->set_userdata((void*)1);
		root->add_child(quit);
		origin_y += (button_height+button_spacing);


//		gui::Panel* root = new gui::Panel(compositor);
//		gui::Label* b = new gui::Label(compositor);
//		b->set_bounds(50, 50, 250, 250);
//		b->set_font(compositor, "fonts/default16");
//		b->set_text("hello");
//		compositor->add_child(b);


//		gui::Panel* root = new gui::Panel(compositor);
//		root->set_bounds(0, 0, 500, 500);
//		compositor->add_child(root);

		graph = new gui::Graph(root);
		graph->set_bounds(params.render_width-250, 0, 250, 250);
		graph->set_font(compositor, "fonts/debug");
		graph->set_background_color(gui::Color(10, 10, 10, 210));
		graph->set_foreground_color(gui::Color(255, 255, 255, 255));
		graph->create_samples(100, 1);
		graph->configure_channel(0, gui::Color(255, 0, 0, 255));
		graph->set_range(-10.0f, 30.0f);
		root->add_child(graph);
		graph->enable_baseline(true, 16.0f, gui::Color(255, 0, 255, 255));
		
		// capture the mouse
//		kernel::instance()->capture_mouse( true );

		kernel::instance()->show_mouse(in_gui);

		// load the game library
		StackString<MAX_PATH_SIZE> game_library_path = ::core::filesystem::content_directory();


		const char* dynamiclibrary_extension = platform::instance()->get_dynamiclibrary_extension();

		game_library_path.append(PATH_SEPARATOR_STRING).append("bin").append(PATH_SEPARATOR_STRING).append("game").append(dynamiclibrary_extension);
		
		gamelib = platform::instance()->open_dynamiclibrary(game_library_path());
		
		if (gamelib == 0)
		{
			LOGV("unable to open game: \"%s\"\n", game_library_path());
			assert(0);
		}
		else
		{
			LOGV("opened game library!\n");
			
			// link the engine interface
			
			connect_engine_fn connect_engine = (connect_engine_fn)platform::instance()->find_dynamiclibrary_symbol(gamelib, "connect_engine");
			disconnect_engine = (disconnect_engine_fn)platform::instance()->find_dynamiclibrary_symbol(gamelib, "disconnect_engine");
			if (connect_engine)
			{
				game_interface = connect_engine(gemini::engine::api::instance());
			}
			if (!game_interface)
			{
				LOGE("Unable to connect engine to game library\n");
				assert(game_interface != 0);
			}
			
			game_interface->startup();
		}
		
		gui_listener.set_game_interface(game_interface);
		gui_listener.set_root(root);
		gui_listener.setup_rendering(params);
		
		center_mouse(params);
		
		// for debugging
		game_interface->level_load();

		return kernel::Application_Success;
	}

	virtual void step( kernel::Parameters& params )
	{
		// this is going to be incorrect unless this is placed in the step.
		// additionally, these aren't interpolated: figure how to; for example,
		// draw hit boxes for a moving player with this system.
		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick( kernel::Parameters& params )
	{
		if (graph)
		{
			graph->record_value(params.framedelta_raw_msec, 0);
		}
	
		if (compositor)
		{
			compositor->update(params.framedelta_raw_msec);
		}
	
		if (!in_gui)
		{
			UserCommand command;
			
			// attempt to sample input here -- may need to be moved.
			bool left = input::state()->keyboard().is_down(input::KEY_A);
			bool right = input::state()->keyboard().is_down(input::KEY_D);
			bool forward = input::state()->keyboard().is_down(input::KEY_W);
			bool back = input::state()->keyboard().is_down(input::KEY_S);
			
			command.set_button(0, left);
			command.set_button(1, right);
			command.set_button(2, forward);
			command.set_button(3, back);
			
			command.set_button(5, input::state()->keyboard().is_down(input::KEY_E));
			command.set_button(6, input::state()->keyboard().is_down(input::KEY_SPACE));
			command.set_button(7, input::state()->keyboard().is_down(input::KEY_LALT) || input::state()->keyboard().is_down(input::KEY_RALT));
			
			command.set_button(8, input::state()->mouse().is_down(input::MOUSE_LEFT));
			command.set_button(9, input::state()->mouse().is_down(input::MOUSE_MIDDLE));
			command.set_button(10, input::state()->mouse().is_down(input::MOUSE_RIGHT));
			
			command.set_button(11, input::state()->keyboard().is_down(input::KEY_G));
			
			int mouse[2];
			kernel::instance()->get_mouse_position(mouse[0], mouse[1]);
			
			int half_width = params.window_width/2;
			int half_height = params.window_height/2;
			
			// capture the state of the mouse
			int mdx, mdy;
			mdx = (mouse[0] - half_width);
			mdy = (mouse[1] - half_height);
			if (mdx != 0 || mdy != 0)
			{
				main_camera.move_view(mdx, mdy);
			}
			
			command.angles[0] = main_camera.pitch;
			command.angles[1] = main_camera.yaw;
			
			
			center_mouse(params);
			
			if (game_interface)
			{
				// loop through all players and process inputs
				game_interface->process_commands(0, &command, 1);
				
				//			game_interface->physics_update(params.step_interval_seconds);
				//			background_source = audio::play(background, 1);
			}
		}
	
	
	
	
//		debugdraw::axes(glm::mat4(1.0), 1.0f);
		int x = 10;
		int y = 10;
		
		if (active_camera)
		{
			debugdraw::text(x, y, core::str::format("active_camera->pos = %.2g %.2g %.2g", active_camera->pos.x, active_camera->pos.y, active_camera->pos.z), Color(255, 255, 255));
			debugdraw::text(x, y+12, core::str::format("eye_position = %.2g %.2g %.2g", active_camera->eye_position.x, active_camera->eye_position.y, active_camera->eye_position.z), Color(255, 0, 255));
			debugdraw::text(x, y+24, core::str::format("active_camera->view = %.2g %.2g %.2g", active_camera->view.x, active_camera->view.y, active_camera->view.z), Color(128, 128, 255));
			debugdraw::text(x, y+36, core::str::format("active_camera->right = %.2g %.2g %.2g", active_camera->side.x, active_camera->side.y, active_camera->side.z), Color(255, 0, 0));
		}
		debugdraw::text(x, y+48, core::str::format("frame delta = %2.2fms\n", params.framedelta_raw_msec), Color(255, 255, 255));
		debugdraw::text(x, y+60, core::str::format("# allocations = %i, total %i Kbytes\n", platform::memory::allocator().active_allocations(), platform::memory::allocator().active_bytes()/1024), Color(64, 102, 192));
		
		
		if (draw_physics_debug)
		{
			physics::debug_draw();
		}

		
		// run server frame
		if (game_interface)
		{
			float framedelta_seconds = params.framedelta_raw_msec*0.001f;
			game_interface->server_frame(params.current_tick, framedelta_seconds, params.step_alpha);
						
			game_interface->client_frame(framedelta_seconds, params.step_alpha);
		}
		

	
//		if (device)
//		{
//			glm::mat4 xform;
//			device->test(xform);
//			debugdraw::axes(xform, 2.0f, 0.1f);
//		}
		
		//glm::mat4 char_mat = glm::mat4(1.0);
		// TODO: this should use the actual player height instead of
		// hard coding the value.
//		char_mat = glm::translate(active_camera->pos - glm::vec3(0,1.82,0));
// needs to be RADIANS now
//		char_mat = glm::rotate(char_mat, -active_camera->yaw, glm::vec3(0,1,0));
		//if (player)
		{
			//player->world_transform = char_mat;
		}
	}
	
	virtual void shutdown( kernel::Parameters& params )
	{
		gui_listener.shutdown();
	
		delete compositor;
		compositor = 0;
	
		DESTROY(SceneRenderMethod, render_method);
		
		if (device)
		{
			vr::destroy_device(device);
		}
		vr::shutdown();
		
		if (game_interface)
		{
			game_interface->shutdown();
		}
		
		if (disconnect_engine)
		{
			disconnect_engine();
		}
		
		platform::instance()->close_dynamiclibrary(gamelib);
	}
};

IMPLEMENT_APPLICATION( ProjectChimera );
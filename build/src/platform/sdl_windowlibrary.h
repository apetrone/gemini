// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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

#include "windowlibrary.h"
#include "input.h"

#include <SDL.h>
#include <map>

namespace platform
{
	struct SDLWindowLibrary : public platform::IWindowLibrary
	{
		// SDL related items
		SDL_Window* window;
		SDL_GLContext context;
		SDL_Rect* display_rects;
		uint8_t total_displays;
		uint8_t total_controllers;
		typedef std::map<unsigned int, input::Button, std::less<unsigned int>> SDLToButtonKeyMap;
		SDLToButtonKeyMap key_map;
		input::MouseButton mouse_map[input::MOUSE_COUNT];
		SDL_GameController* controllers[input::MAX_JOYSTICKS];
		
		
		SDLWindowLibrary();
		
		void startup(kernel::Parameters& parameters);
		
		void populate_input_map();
		void setup_joysticks();
		
		virtual void shutdown();
		virtual void create_window(kernel::Parameters& parameters);
		virtual void process_events();
		virtual void swap_buffers();
		virtual void capture_mouse(bool capture);
		virtual void warp_mouse(int x, int y);
		virtual void get_mouse(int& x, int& y);
		virtual void show_mouse(bool show);
	};
} // namespace platform
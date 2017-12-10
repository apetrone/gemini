// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <stdint.h>
#include <core/mathlib.h>
#include <platform/input.h>
#include <runtime/runtime.h>

namespace kernel
{
	struct KeyboardEvent;
	struct MouseEvent;
	struct SystemEvent;
	struct GameControllerEvent;
} // namespace kernel

namespace gui
{
	class Compositor;
	class Panel;
}

namespace gemini
{
	struct UserCommand;
	struct View;

	struct UserCommand
	{
		int sequence;
		//uint32_t buttonflags;
		float angles[2]; // pitch, yaw
		int16_t axes[4];
		float cam_yaw;
		float cam_pitch;
		glm::quat last_orientation;

		UserCommand()
		{
			sequence = 0;
			//buttonflags = 0;
			angles[0] = angles[1] = 0;
			memset(axes, 0, sizeof(int16_t));
			cam_yaw = cam_pitch = 0.0f;
		}

		//void set_button(int index, bool is_down)
		//{
		//	if (is_down)
		//	{
		//		buttonflags |= (1 << index);
		//	}
		//	else
		//	{
		//		buttonflags &= ~(1 << index);
		//	}
		//}

		void set_axis(uint32_t axis_id, int16_t value)
		{
			axes[axis_id] = value;
		}
	};

	// Camera state which can be interpolated.
#if 0
	// yaw, pitch, roll
	// distance from pivot
	// horizontal / vertical offset
	// field of view

	// there's also horizontal screen offset; as modern games are composing
	// the shots with characters off-center. this isn't interpolated however,
	// so it can be omitted for now.
#endif
	struct CameraState
	{
		glm::vec3 world_position;
		glm::vec3 position;
		glm::quat rotation; // or yaw, pitch, and roll.
		glm::vec3 view; // view direction of camera
		float distance_from_pivot;
		float horizontal_offset;
		float vertical_offset;
		float field_of_view;
	};

	// generate camera space transform from camera_state
	glm::mat4 camera_state_to_transform(const CameraState& camera_state);

	// Describes the interface exposed to the engine from the game.
	class IGameInterface
	{
	public:
		virtual ~IGameInterface() {};

		// called when the engine connects to the game library
		virtual bool startup(gui::Compositor* compositor, gui::Panel* root) = 0;

		// called just before the engine disconnects from the game library
		virtual void shutdown() = 0;

		// called on level change
		virtual void level_load() = 0;

		/// @brief Tick the game
		/// @param current_ticks The tick counter
		/// @param delta_seconds The actual frame time delta (in seconds).
		virtual void tick(uint64_t current_tick, float framedelta_seconds) = 0;

		/// @brief Perform a fixed step at regular interval.
		/// @param current_ticks The tick counter
		/// @param step_interval_seconds The fixed step interval (in seconds).
		virtual void fixed_step(uint64_t current_tick, float step_interval_seconds) = 0;

		/// @brief Render a frame
		/// @param alpha interpolation alpha
		virtual void render_frame(float alpha) = 0;

		// event handling
		virtual void handle_game_message(const InputMessage& message) = 0;

		virtual void reset_events() = 0;

		// get view and projection matrices to render from
		virtual void get_render_view(gemini::View& view, glm::vec3& player_offset) = 0;

		virtual void extract_camera(CameraState* state, glm::vec3* position) = 0;
	}; // GameInterface


	class IClientGameInterface
	{
		virtual void run_frame() = 0;
	}; // IClientGameInterface
} // namespace gemini

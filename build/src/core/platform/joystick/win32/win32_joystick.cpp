// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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

#include "platform_internal.h"

#include <core/logging.h>
#include "kernel.h"
#include <platform/input.h>
using namespace gemini;

#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <windows.h>


#include <Xinput.h>
// XInput version compatibility is a bit shady.
// Windows 8 (XInput 1.4), DirectX SDK(XInput 1.3), Windows Vista(XInput 9.1.0)
// So we're going to try and load the DLLs at runtime ourself.

namespace platform
{
	// XInput supports up to FOUR controllers. At some point I should also
	// add support for Raw Input.
	const size_t GEMINI_WIN32_MAX_JOYSTICKS = 4;
	static_assert(GEMINI_WIN32_MAX_JOYSTICKS == GEMINI_JOYSTICK_MAX_JOYSTICKS, "joystick max count mismatch!");

	// Win32-specific joystick data (stored per joystick)
	struct Win32JoystickData
	{
		uint8_t index;
		uint8_t state;
		DWORD last_packet;
		uint8_t last_button[GAMEPAD_BUTTON_COUNT];
		int16_t last_axes[GEMINI_JOYSTICK_MAX_AXES];

		bool is_connected() const
		{
			return (state & GEMINI_JOYSTICK_CONNECTED) == GEMINI_JOYSTICK_CONNECTED;
		} // is_connected

		bool supports_haptics() const
		{
			return (state & GEMINI_JOYSTICK_SUPPORTS_HAPTICS) == GEMINI_JOYSTICK_SUPPORTS_HAPTICS;
		} // supports_haptics
	};

	static Win32JoystickData _joysticks[GEMINI_WIN32_MAX_JOYSTICKS];

	typedef DWORD(*xinput_getstate_fn)(DWORD, XINPUT_STATE*);
	typedef DWORD(*xinput_setstate_fn)(DWORD, XINPUT_VIBRATION*);

	xinput_getstate_fn xinput_getstate			= nullptr;
	xinput_setstate_fn xinput_setstate			= nullptr;
	platform::DynamicLibrary* xinput_library	= nullptr;

	// The interval at which we check disconnected controllers to see if
	// someone has plugged one in.
	const float GEMINI_WIN32_CONTROLLER_POLL_INTERVAL_SECONDS = MillisecondsPerSecond * 3.0f;

	float _next_controller_check = 0.0f;


	void joystick_startup()
	{
		const char* library_names[] = {
			"xinput1_3.dll",
			"xinput1_4.dll"
		};

		const size_t library_name_count = sizeof(library_names) / sizeof(const char*);

		size_t library_index = 0;
		for (;;)
		{
			xinput_library = platform::dylib_open(library_names[library_index]);
			if (xinput_library)
			{
				LOGV("Located XInput DLL: '%s'\n", library_names[library_index]);
				break;
			}

			// reached end of list
			if (library_index == library_name_count)
			{
				break;
			}
		}

		// Xinput dll couldn't be found!
		assert(xinput_library != nullptr);

		if (!xinput_library)
		{
			LOGE("Failed to locate XInput library DLL. Unable to use controllers.\n");
			return;
		}

		// link up functions we need
		xinput_getstate = static_cast<xinput_getstate_fn>(platform::dylib_find(xinput_library, "XInputGetState"));
		xinput_setstate = static_cast<xinput_setstate_fn>(platform::dylib_find(xinput_library, "XInputSetState"));

		for (uint8_t index = 0; index < GEMINI_WIN32_MAX_JOYSTICKS; ++index)
		{
			memset(&_joysticks[index], 0, sizeof(Win32JoystickData));
			_joysticks[index].index = index;
			_joysticks[index].state |= GEMINI_JOYSTICK_DISCONNECTED;

			// All XInput controllers support haptics.
			_joysticks[index].state |= GEMINI_JOYSTICK_SUPPORTS_HAPTICS;
		}
	}

	void joystick_shutdown()
	{
		if (xinput_library)
		{
			for (size_t index = 0; index < GEMINI_WIN32_MAX_JOYSTICKS; ++index)
			{
				if (_joysticks[index].is_connected() && _joysticks[index].supports_haptics())
				{
					// disable haptics
					joystick_set_force(static_cast<uint32_t>(index), 0);
				}
			}
			platform::dylib_close(xinput_library);
			xinput_library = nullptr;
		}
	}


	void xinput_joystick_update_button(Win32JoystickData* joystick, const XINPUT_GAMEPAD* gamepad, DWORD xinput_button, uint32_t platform_button)
	{
		const uint8_t last_state = (joystick->last_button[platform_button]);
		const int16_t current_state = (gamepad->wButtons & xinput_button) == xinput_button;

		if (last_state != current_state)
		{
			joystick->last_button[platform_button] = static_cast<uint8_t>(current_state > 0);
			kernel::GameControllerEvent gce;
			gce.gamepad_id = joystick->index;
			gce.subtype = kernel::JoystickButton;
			gce.button = static_cast<int>(platform_button);
			gce.is_down = (current_state > 0);
			kernel::event_dispatch(gce);
		}
	}



	void xinput_joystick_update_axis(Win32JoystickData* joystick, int16_t axis_value, uint8_t platform_axis_index, int16_t deadzone)
	{
		if ((axis_value > -deadzone) && (axis_value < deadzone))
		{
			// value lies within the deadzone
			axis_value = 0;
		}

		if (axis_value != joystick->last_axes[platform_axis_index])
		{
			joystick->last_axes[platform_axis_index] = axis_value;

			assert(axis_value <= 32767 || axis_value >= -32768);

			kernel::GameControllerEvent gce;
			gce.gamepad_id = joystick->index;
			gce.subtype = kernel::JoystickAxisMoved;
			gce.axis_id = platform_axis_index;
			gce.axis_value = axis_value;
			kernel::event_dispatch(gce);
		}
	}

	void xinput_joystick_update_triggers(Win32JoystickData* joystick, const XINPUT_GAMEPAD* gamepad)
	{
		// Shift left + AND value to convert to 16-bit.
		const int16_t left_value = (gamepad->bLeftTrigger << 7) | gamepad->bLeftTrigger;
		const int16_t right_value = (gamepad->bRightTrigger << 7) | gamepad->bRightTrigger;

		xinput_joystick_update_axis(joystick, left_value, 0, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		xinput_joystick_update_axis(joystick, right_value, 1, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
	}

	void joystick_update(float delta_milliseconds)
	{
		_next_controller_check -= delta_milliseconds;
		const bool check_controllers = (_next_controller_check < FLT_EPSILON);

		for (size_t index = 0; index < GEMINI_WIN32_MAX_JOYSTICKS; ++index)
		{
			Win32JoystickData* joystick = &_joysticks[index];
			if ((_joysticks[index].state & GEMINI_JOYSTICK_CONNECTED) || check_controllers)
			{
				// From MSDN: For performance reasons, don't call XInputGetState
				// for an 'empty' user slot every frame. We recommend that you space
				// out checks for new controllers every few seconds instead.
				XINPUT_STATE controller_state;
				ZeroMemory(&controller_state, sizeof(XINPUT_STATE));
				DWORD result = xinput_getstate(static_cast<DWORD>(index), &controller_state);
				if (result == ERROR_SUCCESS)
				{
					if (!joystick->is_connected())
					{
						// update state and dispatch message
						joystick->state &= ~GEMINI_JOYSTICK_DISCONNECTED;
						joystick->state |= GEMINI_JOYSTICK_CONNECTED;
						kernel::GameControllerEvent gce;
						gce.gamepad_id = joystick->index;
						gce.subtype = kernel::JoystickConnected;
						kernel::event_dispatch(gce);
					}

					if (joystick->last_packet != controller_state.dwPacketNumber)
					{
						// The packet number has changed; this indicates a
						// controller state change.
						joystick->last_packet = controller_state.dwPacketNumber;

						// transfer state from gamepad to our joystick
						const XINPUT_GAMEPAD* gamepad = &controller_state.Gamepad;
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_A, GAMEPAD_BUTTON_A);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_B, GAMEPAD_BUTTON_B);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_X, GAMEPAD_BUTTON_X);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_Y, GAMEPAD_BUTTON_Y);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_BACK, GAMEPAD_BUTTON_BACK);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_START, GAMEPAD_BUTTON_START);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_LEFT_SHOULDER, GAMEPAD_BUTTON_LEFTSHOULDER);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_RIGHT_SHOULDER, GAMEPAD_BUTTON_RIGHTSHOULDER);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_LEFT_THUMB, GAMEPAD_BUTTON_L2);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_RIGHT_THUMB, GAMEPAD_BUTTON_R2);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_UP, GAMEPAD_BUTTON_DPAD_UP);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_DOWN, GAMEPAD_BUTTON_DPAD_DOWN);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_LEFT, GAMEPAD_BUTTON_DPAD_LEFT);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_RIGHT, GAMEPAD_BUTTON_DPAD_RIGHT);

						// handle left and right triggers as axes
						xinput_joystick_update_triggers(joystick, gamepad);

						// handle left thumb stick
						xinput_joystick_update_axis(joystick, gamepad->sThumbLX, 2, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
						xinput_joystick_update_axis(joystick, gamepad->sThumbLY, 3, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

						// handle right thumb stick
						xinput_joystick_update_axis(joystick, gamepad->sThumbRX, 4, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
						xinput_joystick_update_axis(joystick, gamepad->sThumbRY, 5, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
					}
				}
				else if (joystick->is_connected())
				{
					// update state and dispatch message
					joystick->state &= ~GEMINI_JOYSTICK_CONNECTED;
					joystick->state |= GEMINI_JOYSTICK_DISCONNECTED;
					kernel::GameControllerEvent gce;
					gce.gamepad_id = joystick->index;
					gce.subtype = kernel::JoystickDisconnected;
					kernel::event_dispatch(gce);
				}

			}
		}

		if (check_controllers)
		{
			_next_controller_check = GEMINI_WIN32_CONTROLLER_POLL_INTERVAL_SECONDS;
		}

	} // joystick_update

	uint32_t joystick_max_count()
	{
		return GEMINI_WIN32_MAX_JOYSTICKS;
	} // joystick_max_count

	bool joystick_is_connected(uint32_t index)
	{
		assert(index < GEMINI_WIN32_MAX_JOYSTICKS);
		return _joysticks[index].is_connected();
	} // joystick_is_connected

	bool joystick_supports_haptics(uint32_t index)
	{
		assert(index < GEMINI_WIN32_MAX_JOYSTICKS);
		return _joysticks[index].supports_haptics();
	} // joystick_supports_haptics

	void joystick_set_force(uint32_t index, uint16_t force)
	{
		assert(index < GEMINI_WIN32_MAX_JOYSTICKS);

		if (joystick_supports_haptics(index))
		{
			XINPUT_VIBRATION vibration;
			vibration.wLeftMotorSpeed = force;
			vibration.wRightMotorSpeed = force;
			xinput_setstate(static_cast<DWORD>(index), &vibration);
		}
	} // joystick_set_force

} // namespace platform

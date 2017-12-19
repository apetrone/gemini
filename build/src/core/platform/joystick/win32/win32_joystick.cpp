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
		int16_t last_axes[GAMEPAD_BUTTON_COUNT];

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

	typedef void(*win32_update_joysticks_fn)(float delta_milliseconds);


	// declaration so we can update the function pointer.
	void xinput_update_joysticks(float delta_milliseconds);

	// if xinput is not found, we need to populate the functions with a noop interface.
	void win32_update_joysticks_noop(float)
	{
	}

	xinput_getstate_fn xinput_getstate					= nullptr;
	xinput_setstate_fn xinput_setstate					= nullptr;
	platform::DynamicLibrary* xinput_library			= nullptr;
	win32_update_joysticks_fn win32_update_joysticks	= win32_update_joysticks_noop;

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

		for (size_t library_index = 0; library_index < library_name_count; ++library_index)
		{
			assert(xinput_library == nullptr);
			xinput_library = platform::dylib_open(library_names[library_index]);
			if (xinput_library)
			{
				LOGV("Located XInput DLL: '%s'\n", library_names[library_index]);

				// link up functions we need
				xinput_getstate = static_cast<xinput_getstate_fn>(platform::dylib_find(xinput_library, "XInputGetState"));
				xinput_setstate = static_cast<xinput_setstate_fn>(platform::dylib_find(xinput_library, "XInputSetState"));

				if (xinput_getstate == nullptr)
				{
					LOGW("Could not find symbol \"xinput_getstate\"!\n");
					platform::dylib_close(xinput_library);
					xinput_library = nullptr;
					continue;
				}
				if (xinput_setstate == nullptr)
				{
					LOGW("Could not find symbol \"xinput_setstate\"!\n");
					platform::dylib_close(xinput_library);
					xinput_library = nullptr;
					continue;
				}

				win32_update_joysticks = xinput_update_joysticks;
				break;
			}
		}

		if (!xinput_library)
		{
			// Xinput DLL couldn't be found.
			LOGE("Failed to locate XInput library DLL. Unable to use controllers.\n");
			win32_update_joysticks = win32_update_joysticks_noop;
			return;
		}

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
		assert(platform_button < GAMEPAD_BUTTON_COUNT);

		const uint8_t last_state = (joystick->last_axes[platform_button]);
		const int16_t current_state = (gamepad->wButtons & xinput_button) == xinput_button;

		if (last_state != current_state)
		{
			uint8_t value = static_cast<uint8_t>(current_state);
			joystick->last_axes[platform_button] = current_state;

			KernelEvent kevent;
			kevent.gamepad_id = joystick->index;
			kevent.type = kernel::GameController;
			kevent.subtype = kernel::JoystickButton;
			kevent.axis_id = static_cast<int>(platform_button);
			assert(kevent.axis_id < GAMEPAD_BUTTON_COUNT);
			kevent.axis_value = (current_state > 0) ? 32767 : 0;
			kernel_event_queue(kevent);
		}
	}

	uint8_t xinput_joystick_correct_axis_index(uint8_t platform_axis_index, int16_t* axis_value)
	{
		uint32_t flip_value = 0;

		// If we need to flip the value, the axis index is incremented.
		flip_value = (*axis_value < 0);

		uint8_t axis_mapping[] = {
			GAMEPAD_BUTTON_L2,
			GAMEPAD_BUTTON_R2,
			GAMEPAD_STICK0_AXIS_RIGHT,
			GAMEPAD_STICK0_AXIS_UP,
			GAMEPAD_STICK1_AXIS_RIGHT,
			GAMEPAD_STICK1_AXIS_UP
		};

		assert(platform_axis_index < 6);

		// Interpret this as an unsigned value to normalize it.
		if (flip_value)
		{
			*axis_value = -(*axis_value + 1);
		}

		return axis_mapping[platform_axis_index] + flip_value;
	}

	void xinput_joystick_analog_radial(
		Win32JoystickData* joystick,
		int16_t xvalue,
		int16_t yvalue,
		uint8_t axis_selection, // 0: use x axis, 1: use y axis
		uint8_t platform_axis_index,
		int16_t deadzone)
	{
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ee417001(v=vs.85).aspx#dead_zone
		// http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html

		double magnitude = sqrt(xvalue * xvalue + yvalue * yvalue);

		int16_t value_map[] = {
			xvalue,
			yvalue
		};

		int16_t target_value = value_map[axis_selection];

		float normalized_value = target_value / magnitude;
		if (magnitude > deadzone)
		{
			if (magnitude > 32767)
			{
				magnitude = 32767;
			}

			double fix = ((magnitude - deadzone) / (32767 - deadzone));
			normalized_value = (normalized_value * fix);
		}
		else
		{
			magnitude = 0.0;
			normalized_value = 0.0f;
		}

		int16_t corrected_value = static_cast<int16_t>(normalized_value * 32767.0);

		uint8_t axis_mapping[] = {
			GAMEPAD_BUTTON_L2,
			GAMEPAD_BUTTON_R2,
			GAMEPAD_STICK0_AXIS_RIGHT,
			GAMEPAD_STICK0_AXIS_UP,
			GAMEPAD_STICK1_AXIS_RIGHT,
			GAMEPAD_STICK1_AXIS_UP
		};

		uint8_t pos_index = axis_mapping[platform_axis_index];
		uint8_t neg_index = axis_mapping[platform_axis_index] + 1;

		// If we need to flip the value, the axis index is incremented.
		uint32_t flip_value = (corrected_value < 0);
		uint16_t pos_value = 0;
		uint16_t neg_value = 0;

		// Interpret this as an unsigned value to normalize it.
		if (flip_value)
		{
			corrected_value = -(corrected_value + 1);
			pos_value = 0;
			neg_value = corrected_value;
		}
		else
		{
			pos_value = corrected_value;
			neg_value = 0;
		}

		{
			KernelEvent kevent;
			kevent.type = kernel::GameController;
			kevent.subtype = kernel::JoystickAxisMoved;
			kevent.gamepad_id = joystick->index;
			kevent.axis_id = pos_index;
			kevent.axis_value = pos_value;
			kernel_event_queue(kevent);
		}

		{
			KernelEvent kevent;
			kevent.type = kernel::GameController;
			kevent.subtype = kernel::JoystickAxisMoved;
			kevent.gamepad_id = joystick->index;
			kevent.axis_id = neg_index;
			kevent.axis_value = neg_value;
			kernel_event_queue(kevent);
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

			KernelEvent kevent;
			kevent.gamepad_id = joystick->index;
			kevent.type = kernel::GameController;
			kevent.subtype = kernel::JoystickAxisMoved;
			kevent.axis_id = xinput_joystick_correct_axis_index(platform_axis_index, &axis_value);
			kevent.axis_value = axis_value;
			kernel_event_queue(kevent);
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

	void xinput_update_joysticks(float delta_milliseconds)
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

						KernelEvent kevent;
						kevent.type = kernel::GameController;
						kevent.subtype = kernel::JoystickConnected;
						kevent.gamepad_id = joystick->index;
						kernel_event_queue(kevent);
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
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_LEFT_THUMB, GAMEPAD_BUTTON_L3);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_RIGHT_THUMB, GAMEPAD_BUTTON_R3);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_UP, GAMEPAD_BUTTON_DPAD_UP);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_DOWN, GAMEPAD_BUTTON_DPAD_DOWN);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_LEFT, GAMEPAD_BUTTON_DPAD_LEFT);
						xinput_joystick_update_button(joystick, gamepad, XINPUT_GAMEPAD_DPAD_RIGHT, GAMEPAD_BUTTON_DPAD_RIGHT);

						// handle left and right triggers as axes
						xinput_joystick_update_triggers(joystick, gamepad);

						// handle thumb sticks with radial dead zone
						xinput_joystick_analog_radial(joystick, gamepad->sThumbLX, gamepad->sThumbLY, 0, 2, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
						xinput_joystick_analog_radial(joystick, gamepad->sThumbLX, gamepad->sThumbLY, 1, 3, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

						xinput_joystick_analog_radial(joystick, gamepad->sThumbRX, gamepad->sThumbRY, 0, 4, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
						xinput_joystick_analog_radial(joystick, gamepad->sThumbRX, gamepad->sThumbRY, 1, 5, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
					}
				}
				else if (joystick->is_connected())
				{
					// update state and dispatch message
					joystick->state &= ~GEMINI_JOYSTICK_CONNECTED;
					joystick->state |= GEMINI_JOYSTICK_DISCONNECTED;

					KernelEvent kevent;
					kevent.gamepad_id = joystick->index;
					kevent.type = kernel::GameController;
					kevent.subtype = kernel::JoystickDisconnected;
					kernel_event_queue(kevent);
				}
			}
		}

		if (check_controllers)
		{
			_next_controller_check = GEMINI_WIN32_CONTROLLER_POLL_INTERVAL_SECONDS;
		}
	}


	void joystick_update(float delta_milliseconds)
	{
		win32_update_joysticks(delta_milliseconds);
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

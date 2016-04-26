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
#include "platform_internal.h"
#include "window_provider.h"
#include "graphics_provider.h"
#include "input_provider.h"

#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#if defined(PLATFORM_RASPBERRYPI)
	#include <bcm_host.h> // for bcm_host_init

	#include "../../window/dispmanx/dispmanx_window_provider.h"
#endif



#if defined(PLATFORM_EGL_SUPPORT)
	#include "../../graphics/egl/egl_graphics_provider.h"
#endif

#if defined(PLATFORM_X11_SUPPORT)
	#include "../../window/x11/x11_window_provider.h"
	#include "../../graphics/x11/x11_graphics_provider.h"
#endif


// for evdev
#include <core/logging.h>
#include <fcntl.h>
#include <linux/input.h>
#include <libudev.h>

//idVendor:idProduct
// 045e:028e -- Xbox360 controller

// etc/udev/rules.d/99-input.rules
// KERNEL=="mouse*|event*|mice", MODE="0777"
// KERNEL=="event[0-9]*", ENV{ID_BUS}=="?*", ENV{ID_INPUT_JOYSTICK}=="?*", GROUP="games", MODE="0660"
// KERNEL=="js[0-9]*", ENV{ID_BUS}=="?*", ENV{ID_INPUT_JOYSTICK}=="?*", GROUP="games", MODE="0664"

// reload rules: (as superuser)
// udevadm control --reload
// udevadm trigger

// https://wiki.archlinux.org/index.php/Gamepad#PlayStation_4_controller

const size_t MAX_KEYBOARD_KEYS = 250;
const size_t DEVICE_BUFFER_SIZE = sizeof(struct input_event) * 64;
enum class DeviceType
{
	Keyboard,
	Mouse,
	Joystick
};
class InputDevice
{
public:
	virtual ~InputDevice();
	virtual int get_descriptor() const = 0;
	virtual const char* get_device_path() const = 0;
	virtual DeviceType get_device_type() const = 0;
	virtual bool process_event(const struct input_event&) = 0;
};

InputDevice::~InputDevice()
{
}


bool device_is_supported(const char* device_path)
{
	// try to open the device
	int descriptor = open(device_path, O_RDONLY);
	if (descriptor < 0)
	{
		LOGW("Skipping device '%s' (cannot open)\n", device_path);
		return false;
	}

	// if we can open it; try to get the EVIO version
	int version;
	if (ioctl(descriptor, EVIOCGVERSION, &version) != 0)
	{
		LOGW("Skipping device '%s' (EVIOCGVERSION)\n", device_path);
		return false;
	}

	// close the device, checks out.
	close(descriptor);

	return true;
}

template <DeviceType T>
class InputDeviceBase : public InputDevice
{
public:
	InputDeviceBase(const char* device_path_node) :
		descriptor(-1),
		device_path(device_path_node)
	{
		LOGV("opening: '%s'...\n", device_path_node);
		descriptor = open(device_path_node, O_RDONLY);
		if (descriptor < 0)
		{
			LOGW("%s (%i), Unable to open device '%s'\n",
				strerror(errno),
				errno,
				device_path_node
			);
		}
		assert(descriptor != -1);

		int version;
		if (ioctl(descriptor, EVIOCGVERSION, &version))
		{
			LOGW("Can't get version: %i\n", errno);
		}
		LOGV("driver version: %i.%i.%i\n",
			version >> 16,
			(version >> 8) & 0xff,
			version & 0xff
		);

		char name[256] = {0};
		ioctl(descriptor, EVIOCGNAME(256), name);
		LOGV("device name: %s\n", name);
	}

	virtual ~InputDeviceBase()
	{
		if (descriptor != -1)
		{
			close(descriptor);
			descriptor = -1;
		}
	}

	virtual int get_descriptor() const
	{
		return descriptor;
	}

	virtual const char* get_device_path() const
	{
		return device_path();
	}

	virtual DeviceType get_device_type() const
	{
		return T;
	}

protected:
	int descriptor;
	core::StackString<32> device_path;
};

class KeyboardDevice : public InputDeviceBase<DeviceType::Keyboard>
{
public:
	KeyboardDevice(const char* device_path) : InputDeviceBase(device_path)
	{
		using namespace platform;
		memset(keymap, 0, MAX_KEYBOARD_KEYS);

		keymap[KEY_ESC] = gemini::BUTTON_ESCAPE;

		keymap[KEY_1] = gemini::BUTTON_1;
		keymap[KEY_2] = gemini::BUTTON_2;
		keymap[KEY_3] = gemini::BUTTON_3;
		keymap[KEY_4] = gemini::BUTTON_4;
		keymap[KEY_5] = gemini::BUTTON_5;
		keymap[KEY_6] = gemini::BUTTON_6;
		keymap[KEY_7] = gemini::BUTTON_7;
		keymap[KEY_8] = gemini::BUTTON_8;
		keymap[KEY_9] = gemini::BUTTON_9;
		keymap[KEY_0] = gemini::BUTTON_0;
		keymap[KEY_MINUS] = gemini::BUTTON_MINUS;
		keymap[KEY_EQUAL] = gemini::BUTTON_EQUALS;
		keymap[KEY_BACKSPACE] = gemini::BUTTON_BACKSPACE;
		keymap[KEY_TAB] = gemini::BUTTON_TAB;
		keymap[KEY_Q] = gemini::BUTTON_Q;
		keymap[KEY_W] = gemini::BUTTON_W;
		keymap[KEY_E] = gemini::BUTTON_E;
		keymap[KEY_R] = gemini::BUTTON_R;
		keymap[KEY_T] = gemini::BUTTON_T;
		keymap[KEY_Y] = gemini::BUTTON_Y;
		keymap[KEY_U] = gemini::BUTTON_U;
		keymap[KEY_I] = gemini::BUTTON_I;
		keymap[KEY_O] = gemini::BUTTON_O;
		keymap[KEY_P] = gemini::BUTTON_P;
		keymap[KEY_LEFTBRACE] = gemini::BUTTON_LBRACKET;
		keymap[KEY_RIGHTBRACE] = gemini::BUTTON_RBRACKET;
		keymap[KEY_ENTER] = gemini::BUTTON_RETURN;
		keymap[KEY_LEFTCTRL] = gemini::BUTTON_LCONTROL;
		keymap[KEY_A] = gemini::BUTTON_A;
		keymap[KEY_S] = gemini::BUTTON_S;
		keymap[KEY_D] = gemini::BUTTON_D;
		keymap[KEY_F] = gemini::BUTTON_F;
		keymap[KEY_G] = gemini::BUTTON_G;
		keymap[KEY_H] = gemini::BUTTON_H;
		keymap[KEY_J] = gemini::BUTTON_J;
		keymap[KEY_K] = gemini::BUTTON_K;
		keymap[KEY_L] = gemini::BUTTON_L;
		keymap[KEY_SEMICOLON] = gemini::BUTTON_SEMICOLON;
		keymap[KEY_APOSTROPHE] = gemini::BUTTON_TILDE;
		keymap[KEY_GRAVE] = gemini::BUTTON_INVALID;
		keymap[KEY_LEFTSHIFT] = gemini::BUTTON_LSHIFT;
		keymap[KEY_BACKSLASH] = gemini::BUTTON_RSHIFT;
		keymap[KEY_Z] = gemini::BUTTON_Z;
		keymap[KEY_X] = gemini::BUTTON_X;
		keymap[KEY_C] = gemini::BUTTON_C;
		keymap[KEY_V] = gemini::BUTTON_V;
		keymap[KEY_B] = gemini::BUTTON_B;
		keymap[KEY_N] = gemini::BUTTON_N;
		keymap[KEY_M] = gemini::BUTTON_M;
		keymap[KEY_COMMA] = gemini::BUTTON_COMMA;
		keymap[KEY_DOT] = gemini::BUTTON_PERIOD;
		keymap[KEY_SLASH] = gemini::BUTTON_SLASH;
		keymap[KEY_RIGHTSHIFT] = gemini::BUTTON_RSHIFT;
		keymap[KEY_KPASTERISK] = gemini::BUTTON_NUMPAD_MULTIPLY;
		keymap[KEY_LEFTALT] = gemini::BUTTON_LALT;
		keymap[KEY_SPACE] = gemini::BUTTON_SPACE;
		keymap[KEY_CAPSLOCK] = gemini::BUTTON_CAPSLOCK;
		keymap[KEY_F1] = gemini::BUTTON_F1;
		keymap[KEY_F2] = gemini::BUTTON_F2;
		keymap[KEY_F3] = gemini::BUTTON_F3;
		keymap[KEY_F4] = gemini::BUTTON_F4;
		keymap[KEY_F5] = gemini::BUTTON_F5;
		keymap[KEY_F6] = gemini::BUTTON_F6;
		keymap[KEY_F7] = gemini::BUTTON_F7;
		keymap[KEY_F8] = gemini::BUTTON_F8;
		keymap[KEY_F9] = gemini::BUTTON_F9;
		keymap[KEY_F10] = gemini::BUTTON_F10;
		keymap[KEY_NUMLOCK] = gemini::BUTTON_NUMLOCK;
		keymap[KEY_SCROLLLOCK] = gemini::BUTTON_SCROLLLOCK;
		keymap[KEY_KP7] = gemini::BUTTON_NUMPAD7;
		keymap[KEY_KP8] = gemini::BUTTON_NUMPAD8;
		keymap[KEY_KP9] = gemini::BUTTON_NUMPAD9;
		keymap[KEY_KPMINUS] = gemini::BUTTON_NUMPAD_MINUS;
		keymap[KEY_KP4] = gemini::BUTTON_NUMPAD4;
		keymap[KEY_KP5] = gemini::BUTTON_NUMPAD5;
		keymap[KEY_KP6] = gemini::BUTTON_NUMPAD6;
		keymap[KEY_KPPLUS] = gemini::BUTTON_NUMPAD_PLUS;
		keymap[KEY_KP1] = gemini::BUTTON_NUMPAD1;
		keymap[KEY_KP2] = gemini::BUTTON_NUMPAD2;
		keymap[KEY_KP3] = gemini::BUTTON_NUMPAD3;
		keymap[KEY_KP0] = gemini::BUTTON_NUMPAD0;
		keymap[KEY_KPDOT] = gemini::BUTTON_NUMPAD_PERIOD;

		keymap[KEY_ZENKAKUHANKAKU] = gemini::BUTTON_INVALID;
		keymap[KEY_102ND] = -1;
		keymap[KEY_F11] = gemini::BUTTON_F11;
		keymap[KEY_F12] = gemini::BUTTON_F12;

		keymap[KEY_KPENTER] = gemini::BUTTON_NUMPAD_ENTER;
		keymap[KEY_RIGHTCTRL] = gemini::BUTTON_RCONTROL;
		keymap[KEY_KPSLASH] = gemini::BUTTON_NUMPAD_DIVIDE;
		keymap[KEY_RIGHTALT] = gemini::BUTTON_RALT;

		keymap[KEY_HOME] = gemini::BUTTON_HOME;
		keymap[KEY_UP] = gemini::BUTTON_UP;
		keymap[KEY_PAGEUP] = gemini::BUTTON_PAGEUP;
		keymap[KEY_LEFT] = gemini::BUTTON_LEFT;
		keymap[KEY_RIGHT] = gemini::BUTTON_RIGHT;
		keymap[KEY_END] = gemini::BUTTON_END;
		keymap[KEY_DOWN] = gemini::BUTTON_DOWN;
		keymap[KEY_PAGEDOWN] = gemini::BUTTON_PAGEDN;
		keymap[KEY_INSERT] = gemini::BUTTON_INSERT;
		keymap[KEY_DELETE] = gemini::BUTTON_DELETE;
		// keymap[KEY_MACRO] = gemini::BUTTON_MACRO;
		// keymap[KEY_MUTE] = gemini::BUTTON_MUTE;
		// keymap[KEY_VOLUMEDOWN] = gemini::BUTTON_VOLUMEDOWN;
		// keymap[KEY_VOLUMEUP] = gemini::BUTTON_VOLUMEUP;
		//keymap[KEY_POWER]
		keymap[KEY_KPEQUAL] = gemini::BUTTON_NUMPAD_EQUALS;
		keymap[KEY_KPPLUSMINUS] = gemini::BUTTON_NUMPAD_PLUSMINUS;
		keymap[KEY_PAUSE] = gemini::BUTTON_PAUSE;
		//keymap[KEY_SCALE]
		//keymap[KEY_KPCOMMA]
		//keymap[KEY_HANGEUL]
		//keymap[KEY_HANGUEL]
		//keymap[KEY_HANJA]
		//keymap[KEY_YEN]
		keymap[KEY_LEFTMETA] = gemini::BUTTON_LOSKEY;
		keymap[KEY_RIGHTMETA] = gemini::BUTTON_ROSKEY;
		// keymap[KEY_COMPOSE]
		// keymap[KEY_STOP]
		// keymap[KEY_AGAIN]
		// keymap[KEY_PROPS]
		// keymap[KEY_UNDO]
		// keymap[KEY_FRONT]
		// keymap[KEY_COPY]
		// keymap[KEY_OPEN]
		// keymap[KEY_PASTE]
		// keymap[KEY_FIND]
		// keymap[KEY_CUT]
		// keymap[KEY_HELP]
		// keymap[KEY_MENU]
		// keymap[KEY_CALC]
		// keymap[KEY_SETUP]
		// keymap[KEY_SLEEP]
		// keymap[KEY_WAKEUP]
		// keymap[KEY_FILE]
		// keymap[KEY_SENDFILE]
		// keymap[KEY_DELETEFILE]
		// keymap[KEY_XFER]
		// keymap[KEY_PROG1]
		// keymap[KEY_PROG2]
		// keymap[KEY_WWW]
		// keymap[KEY_MSDOS]
		// keymap[KEY_COFFEE]
		// keymap[KEY_SCREENLOCK]
		// keymap[KEY_ROTATE_DISPLAY]
		// keymap[KEY_DIRECTION]
		// keymap[KEY_CYCLEWINDOWS]
		// keymap[KEY_MAIL]
		// keymap[KEY_BOOKMARKS]
		// keymap[KEY_COMPUTER]
		// keymap[KEY_BACK]
		// keymap[KEY_FORWARD]
		// keymap[KEY_CLOSECD]
		// keymap[KEY_EJECTCD]
		// keymap[KEY_EJECTCLOSECD]
		// keymap[KEY_NEXTSONG]
		// keymap[KEY_PLAYPAUSE]
		// keymap[KEY_PREVIOUSSONG]
		// keymap[KEY_STOPCD]
		// keymap[KEY_RECORD]
		// keymap[KEY_REWIND]
		// keymap[KEY_PHONE]
		// keymap[KEY_ISO]
		// keymap[KEY_CONFIG]
		// keymap[KEY_HOMEPAGE]
		// keymap[KEY_REFRESH]
		// keymap[KEY_EXIT]
		// keymap[KEY_MOVE]
		// keymap[KEY_EDIT]
		// keymap[KEY_SCROLLUP]
		// keymap[KEY_SCROLLDOWN]
		// keymap[KEY_KPLEFTPAREN]
		// keymap[KEY_KPRIGHTPAREN]
		// keymap[KEY_NEW]
		// keymap[KEY_REDO]
		keymap[KEY_F13] = gemini::BUTTON_F13;
		keymap[KEY_F14] = gemini::BUTTON_F14;
		keymap[KEY_F15] = gemini::BUTTON_F15;
		keymap[KEY_F16] = gemini::BUTTON_F16;
		keymap[KEY_F17] = gemini::BUTTON_F17;
		keymap[KEY_F18] = gemini::BUTTON_F18;
		keymap[KEY_F19] = gemini::BUTTON_F19;
		keymap[KEY_F20] = gemini::BUTTON_F20;
		keymap[KEY_F21] = gemini::BUTTON_F21;
		keymap[KEY_F22] = gemini::BUTTON_F22;
		keymap[KEY_F23] = gemini::BUTTON_F23;
		keymap[KEY_F24] = gemini::BUTTON_F24;
	}

	virtual bool process_event(const struct input_event& event)
	{
		switch(event.type)
		{
			case EV_KEY:
			// code: corresponding KEY_* code.
			// value: 0 for up, 1 for down, 2 for held.
			//

			// key input
			if (event.value < 2)
			{
				printf("type: %x, code: %d, value: %d -> %i\n",
					event.type, event.code, event.value, keymap[event.code]);
			}
				break;

			// ignore these for now
			case EV_SYN: // synchronization event
			case EV_MSC: // misc events
			case EV_LED: // used to turn leds on and off
				break;

			default:
				printf("unhandled event.type: %x\n", event.type);
				break;
		}

		return false;
	}

private:
	uint32_t keymap[MAX_KEYBOARD_KEYS];
};

class MouseDevice : public InputDeviceBase<DeviceType::Keyboard>
{
public:
	MouseDevice(const char* device_path) : InputDeviceBase(device_path)
	{
	}

	virtual bool process_event(const struct input_event& event)
	{
		printf("type: %d, code: %d, value: %d\n",
			event.type, event.code, event.value);
		if (event.type == EV_REL)
		{
			// EV_REL
			// REL_WHEEL, REL_HWHEEL

		}
		else if (event.type == EV_ABS)
		{
			// ABS_DISTANCE
			// ABS_MT_*: multi-touch input events
		}
		else if (event.type == EV_KEY)
		{
			// BTN_*
		}
		else
		{
			printf("unhandled type: %d\n", event.type);
		}
		return false;
	}
};

// on applying force-feedback
// https://www.kernel.org/doc/Documentation/input/ff.txt
// http://freegamedev.net/wiki/Input_Handling
class JoystickDevice : public InputDeviceBase<DeviceType::Joystick>
{
public:

	JoystickDevice(const char* device_path) : InputDeviceBase(device_path)
	{
		// TODO: We don't need a button map per joystick
		// make this static!
		memset(buttonmap, gemini::GAMEPAD_BUTTON_INVALID, 128);

		set_button(BTN_MISC, gemini::GAMEPAD_BUTTON_INVALID);
		set_button(BTN_A, gemini::GAMEPAD_BUTTON_A);
		set_button(BTN_B, gemini::GAMEPAD_BUTTON_B);
		set_button(BTN_C, gemini::GAMEPAD_BUTTON_C);
		set_button(BTN_X, gemini::GAMEPAD_BUTTON_X);
		set_button(BTN_Y, gemini::GAMEPAD_BUTTON_Y);
		set_button(BTN_Z, gemini::GAMEPAD_BUTTON_Z);
		set_button(BTN_THUMBL, gemini::GAMEPAD_BUTTON_LEFTSTICK);
		set_button(BTN_THUMBR, gemini::GAMEPAD_BUTTON_RIGHTSTICK);
		set_button(BTN_TL, gemini::GAMEPAD_BUTTON_LEFTSHOULDER);
		set_button(BTN_TR, gemini::GAMEPAD_BUTTON_RIGHTSHOULDER);
		set_button(BTN_TL2, gemini::GAMEPAD_BUTTON_L2);
		set_button(BTN_TR2, gemini::GAMEPAD_BUTTON_R2);
		set_button(BTN_SELECT, gemini::GAMEPAD_BUTTON_SELECT);
		set_button(BTN_START, gemini::GAMEPAD_BUTTON_START);
		set_button(BTN_MODE, gemini::GAMEPAD_BUTTON_GUIDE);
	}

	void set_button(int code, gemini::GamepadButton button)
	{
		buttonmap[code-BTN_MISC] = button;
	}

	inline gemini::GamepadButton get_button(int code)
	{
		return buttonmap[code-BTN_MISC];
	}

	virtual bool process_event(const struct input_event& event)
	{
		if (event.type == EV_SYN)
		{
			// printf("EVN_SYN\n");
		}
		else if (event.type == EV_MSC && (event.code == MSC_RAW || event.code == MSC_SCAN))
		{

		}
		else
		{
			switch(event.type)
			{
				// handle button presses
				case EV_KEY:
				{
					const gemini::GamepadButton button = get_button(event.code);

					if (button > gemini::GAMEPAD_BUTTON_INVALID && (button < gemini::GAMEPAD_BUTTON_COUNT))
					{
						printf("button %s -> %i\n", event.value ? "down" : "up", static_cast<int>(button));
					}
					else
					{
						printf("type: %x, code: %x, value: %d\n",
							event.type, event.code, event.value);
					}
					break;
				}

				// handle axes
				case EV_ABS:
				{
					// event.code is axis_id
					// event.value is axis value [-32767,32767]
					// DPAD are also treated as axes on XboxOne controller.
					// axes 16, 17, with [-1, 1] range
					printf("type: %d, code: %d, value: %d\n", event.type, event.code, event.value);
					break;
				}
			}
		}

		// if (event.type == EV_REL)
		// {
		// 	// EV_REL
		// 	// REL_WHEEL, REL_HWHEEL

		// }
		// else if (event.type == EV_ABS)
		// {
		// 	// ABS_DISTANCE
		// 	// ABS_MT_*: multi-touch input events
		// }
		// else if (event.type == EV_KEY)
		// {
		// 	// BTN_*
		// }
		// else
		// {
		// 	printf("unhandled type: %d\n", event.type);
		// }
		return false;
	}

private:
	gemini::GamepadButton buttonmap[128];
};

class EVDevInputProvider : public platform::InputProvider
{


	Array<InputDevice*> devices;

private:
	void create_device_with_path(const char* device_path,
								DeviceType device_type)
	{
		InputDevice* device = nullptr;
		switch(device_type)
		{
			case DeviceType::Keyboard:
				device = MEMORY_NEW(KeyboardDevice, core::memory::global_allocator())(device_path);
				break;
			case DeviceType::Mouse:
				device = MEMORY_NEW(MouseDevice, core::memory::global_allocator())(device_path);
				break;
			case DeviceType::Joystick:
				device = MEMORY_NEW(JoystickDevice, core::memory::global_allocator())(device_path);
				break;
			default:
				LOGE("Unknown device type specified!\n");
				assert(0);
				break;
		}

		assert(device != nullptr);
		devices.push_back(device);
	}

	void add_devices(const char* subsystem,
					const char* property_name,
					const char* property_value,
					DeviceType device_type)
	{
		// http://www.signal11.us/oss/udev/
		struct udev* lib;
		struct udev_enumerate* enumerate;
		struct udev_list_entry* devices;
		struct udev_list_entry* entry;

		// create
		lib = udev_new();
		if (!lib)
		{
			LOGE("cannot create the udev lib\n");
			return;
		}

		// create a list of devices in the 'hidraw' subsystem
		enumerate = udev_enumerate_new(lib);

		// LOGV("looking for subsystem matches: '%s'\n", subsystem);
		udev_enumerate_add_match_subsystem(enumerate, subsystem);
		if (property_name && property_value)
		{
			udev_enumerate_add_match_property(enumerate,
				property_name,
				property_value
			);
		}
		udev_enumerate_scan_devices(enumerate);
		devices = udev_enumerate_get_list_entry(enumerate);

		udev_list_entry_foreach(entry, devices)
		{
			const char* sys_device_path;

			// get the filename of the /sys entry for the device
			// and create a udev_device object representing it.
			sys_device_path = udev_list_entry_get_name(entry);
			// LOGV("name: %s\n",
			// 	sys_device_path);

			udev_device* udevice = udev_device_new_from_syspath(lib, sys_device_path);
			if (!udevice)
			{
				LOGW("Unable to find device for sys path: %s\n",
					sys_device_path
				);
				continue;
			}

			// get the path of the device in the /dev/ tree
			const char* device_node_path = udev_device_get_devnode(udevice);
			if (!device_node_path)
			{
				LOGV("Unable to get device node path. Skipping device...\n");
				udev_device_unref(udevice);
				continue;
			}

			// The device pointed to by dev contains info about the hidraw
			// device. In order to get info about the device, get the
			// parent usb device with the subsystem/devtype pair of
			// "usb"/"usb_device". This will be several levels up the
			// tree, but the function will find it.
			udev_device* uparent = udev_device_get_parent_with_subsystem_devtype(
				udevice,
				"usb",
				"usb_device");

			if (!uparent)
			{
				LOGV("Unable to find parent usb device. Skipping device...\n");
				udev_device_unref(udevice);
				continue;
			}

			// usb_device_get_devnode() returns the path to the device node
			// in /dev/
			LOGV("Device Node Path: %s\n", device_node_path);

			// from here, call get_sysattr_value for each file in the
			// device's /sys entry. The strings passed into these
			// functions (idProduct, idVendor, serial, etc.) correspond
			// directly to the files in the directory representing the
			// usb device. Note that USB strings are Unicode, UCS2
			// encoded, but the strings returned from
			// udev_device_get_sysattr_value are UTF-8 encoded.
			LOGV(
				"VID/PID: %s %s\n",
				udev_device_get_sysattr_value(uparent, "idVendor"),
				udev_device_get_sysattr_value(uparent, "idProduct")
			);

			LOGV(
				"manufacturer: %s\nproduct: %s\n",
				udev_device_get_sysattr_value(uparent, "manufacturer"),
				udev_device_get_sysattr_value(uparent, "product")
			);

			LOGV(
				"serial: %s\n",
				udev_device_get_sysattr_value(uparent, "serial")
			);

			LOGV(
				"bustype: %i\n",
				udev_device_get_sysattr_value(uparent, "idBustype")
			);

			if (device_is_supported(device_node_path))
			{
				// register this device type before we unref devices
				// as they take the device_node_path with them.
				create_device_with_path(device_node_path, device_type);
			}

			// free resources
			//udev_device_unref(uparent); // this causes a crash; do we need
			// to free the parent here?
			udev_device_unref(udevice);
		}
		udev_enumerate_unref(enumerate);
		udev_unref(lib);
	}

public:
	virtual platform::Result startup() override
	{
		// scan for and add devices of known types
		//
		add_devices("input", "ID_INPUT_KEYBOARD", "1", DeviceType::Keyboard);
		add_devices("input", "ID_INPUT_MOUSE", "1", DeviceType::Mouse);
		add_devices("input", "ID_INPUT_JOYSTICK", "1", DeviceType::Joystick);

		return platform::Result::success();
	}

	virtual void shutdown() override
	{
		for (InputDevice* device : devices)
		{
			MEMORY_DELETE(device, core::memory::global_allocator());
		}
		devices.clear();
	}

	virtual void update(float delta_milliseconds) override
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);

		int highest_fd = 0;

		for (InputDevice* device : devices)
		{
			assert(device->get_descriptor() != -1);

			FD_SET(device->get_descriptor(), &read_fds);
			if (device->get_descriptor() > highest_fd)
			{
				highest_fd = device->get_descriptor();
			}
		}

		// Use the timeout value as a polling mechanism
		// by setting the values to zero.
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		const int select_result = select(highest_fd + 1, &read_fds, 0, 0, &timeout);
		if (select_result > 0)
		{
			for (InputDevice* device : devices)
			{
				if (device->get_descriptor() > 0)
				{
					if (FD_ISSET(device->get_descriptor(), &read_fds))
					{
						struct input_event buffer[64] = {0};
						int read_bytes = 0;
						read_bytes = read(device->get_descriptor(), buffer, DEVICE_BUFFER_SIZE);
						if (read_bytes < sizeof(struct input_event))
						{
							LOGW("read failed with errno: %i\n", errno);
							break;
						}

						const int total_events_read = (read_bytes / sizeof(struct input_event));
						//LOGV("read %i bytes, %i events\n", read_bytes, total_events_read);
						for (int index = 0; index < total_events_read; ++index)
						{
							device->process_event(buffer[index]);
						}
					}
				}
			}
		}
		else if (select_result == -1)
		{
			LOGW("%s (%i) - Error polling input devices",
				strerror(errno),
				errno
			);
		}
	}
};

#include <stdio.h> // for fprintf

using namespace platform::window;

namespace platform
{
	namespace linux
	{
		static WindowProvider* _window_provider = nullptr;
		static GraphicsProvider* _graphics_provider = nullptr;
		static InputProvider* _input_provider = nullptr;

		// choose the best window provider
		WindowProvider* create_window_provider()
		{
#if defined(PLATFORM_RASPBERRYPI)
			typedef DispManXWindowProvider window_provider_type;
#elif defined(PLATFORM_X11_SUPPORT)
			typedef X11WindowProvider window_provider_type;
#else
			#error No window provider for this platform!
#endif

			return MEMORY_NEW(window_provider_type, get_platform_allocator());
		}

		GraphicsProvider* create_graphics_provider()
		{
#if defined(PLATFORM_RASPBERRYPI)
			typedef EGLGraphicsProvider graphics_provider_type;
#elif defined(PLATFORM_X11_SUPPORT)
			typedef X11GraphicsProvider graphics_provider_type;
#else
			#error No graphics provider for this platform!
#endif

			return MEMORY_NEW(graphics_provider_type, get_platform_allocator());
		}

		InputProvider* create_input_provider()
		{
			typedef EVDevInputProvider input_provider_type;

			return MEMORY_NEW(input_provider_type, get_platform_allocator());
		}

	} // namespace linux


	using namespace platform::linux;

	Result backend_startup()
	{
		//
		// input provider
		assert(_input_provider == nullptr);
		_input_provider = create_input_provider();
		if (!_input_provider)
		{
			return Result::failure("create_input_provider failed!");
		}

		Result input_startup = _input_provider->startup();
		if (input_startup.failed())
		{
			fprintf(stderr, "input_provider startup failed: %s\n", input_startup.message);
			return input_startup;
		}

		return Result::success();
	}

	void backend_shutdown()
	{
		assert(_input_provider != nullptr);
		_input_provider->shutdown();
		MEMORY_DELETE(_input_provider, get_platform_allocator());
	}

	void backend_update(float delta_milliseconds)
	{
		_input_provider->update(delta_milliseconds);

		platform::window::dispatch_events();
	}

	Process* process_create(const char* /*executable_path*/,
		const Array<PathString>& /*arguments*/,
		const char* /*working_directory*/)
	{
		return nullptr;
	}

	void process_destroy(Process* /*process*/)
	{

	}

	bool process_is_running(Process* /*process*/)
	{
		return false;
	}

	namespace window
	{
		Result startup(RenderingBackend backend)
		{
			// On linux, the platform layer needs to be versatile.
			// It is split into three main components I call 'providers':
			// 1. Window: platform window management layer
			// 2. Graphics: rendering context provider
			// 3. Input: various input systems

			// Ultimately, we have to choose the best window provider
			// either via build settings or at runtime.

#if defined(PLATFORM_RASPBERRYPI)
			// this must be called before we can issue any hardware commands
			bcm_host_init();
#endif

			// window provider
			assert(_window_provider == nullptr);
			_window_provider = create_window_provider();
			if (!_window_provider)
			{
				return Result::failure("create_window_provider failed!");
			}

			Result window_startup = _window_provider->startup();
			if (window_startup.failed())
			{
				fprintf(stderr, "window_provider startup failed: %s\n", window_startup.message);
				return window_startup;
			}

			//
			// graphics provider
			assert(_graphics_provider == nullptr);
			_graphics_provider = create_graphics_provider();
			if (!_graphics_provider)
			{
				return Result::failure("create_graphics_provider failed!");
			}

			Result graphics_startup = _graphics_provider->startup(_window_provider);
			if (graphics_startup.failed())
			{
				fprintf(stderr, "graphics_provider startup failed: %s\n", graphics_startup.message);
				return graphics_startup;
			}

#if defined(PLATFORM_RASPBERRYPI)
			// force the backend
			backend = RenderingBackend_OpenGLES2;

			if (backend != RenderingBackend_OpenGLES2)
			{
				return Result::failure("The only supported rendering backend is OpenGLES 2");
			}
#else
			// force OpenGL for now
			backend = RenderingBackend_OpenGL;
			if (backend != RenderingBackend_OpenGL)
			{
				return Result::failure("The only supported rendering backend is OpenGL");
			}
#endif
			return Result::success();
		}

		void shutdown()
		{
			assert(_graphics_provider != nullptr);
			_graphics_provider->shutdown(_window_provider);
			MEMORY_DELETE(_graphics_provider, get_platform_allocator());

			assert(_window_provider != nullptr);
			_window_provider->shutdown();
			MEMORY_DELETE(_window_provider, get_platform_allocator());
			_window_provider = nullptr;

#if defined(PLATFORM_RASPBERRYPI)
			bcm_host_deinit();
#endif
		}

		void dispatch_events()
		{
			_window_provider->dispatch_events();
		}

		NativeWindow* create(const Parameters& window_parameters)
		{
			// allocate data for the graphics provider
			size_t graphics_data_size = _graphics_provider->get_graphics_data_size();
			void* graphics_data = nullptr;

			if (graphics_data_size)
			{
				// alloc graphics data for this window
				graphics_data = MEMORY_ALLOC(graphics_data_size, get_platform_allocator());
			}

			// have the graphics provider figure out what it may need prior
			// to window creation.
			_graphics_provider->pre_window_creation(window_parameters, graphics_data);

			// create the native window and assign the graphics data
			NativeWindow* window = _window_provider->create(window_parameters, _graphics_provider->get_native_visual(graphics_data));
			window->graphics_data = graphics_data;


			window->backbuffer = window_parameters.backbuffer;

			// pass the window to the graphics API for context creation
			_graphics_provider->create_context(window);

			// another pass to create the 'surface'
			_graphics_provider->create_surface(window);

			// activate the context for newly created windows
			_graphics_provider->attach_context(window);

			return window;
		}

		void destroy(NativeWindow* window, DestroyWindowBehavior behavior)
		{
			_graphics_provider->detach_context(window);
			_graphics_provider->destroy_surface(window);
			_graphics_provider->destroy_context(window);

			MEMORY_DEALLOC(window->graphics_data, get_platform_allocator());
			_window_provider->destroy(window, behavior);
		}

		void activate_context(NativeWindow* window)
		{
			_graphics_provider->attach_context(window);
		}

		void deactivate_context(NativeWindow* window)
		{
			_graphics_provider->detach_context(window);
		}

		void swap_buffers(NativeWindow* window)
		{
			_graphics_provider->swap_buffers(window);
		}

		Frame get_frame(NativeWindow* window)
		{
			return _window_provider->get_frame(window);
		}

		Frame get_render_frame(NativeWindow* window)
		{
			return _window_provider->get_render_frame(window);
		}

		size_t screen_count()
		{
			return _window_provider->get_screen_count();
		}

		Frame screen_frame(size_t screen_index)
		{
			return _window_provider->get_screen_frame(screen_index);
		}

		void focus(NativeWindow* /*window*/)
		{
		}

		void show_cursor(bool /*enable*/)
		{
		}

		void set_cursor(float /*x*/, float /*y*/)
		{

		}

		void get_cursor(float& /*x*/, float& /*y*/)
		{

		}

		void set_relative_mouse_mode(NativeWindow* /*native_window*/, bool /*enable*/)
		{

		}

		void set_mouse_tracking(bool /*enable*/)
		{

		}
	} // namespace window


	size_t system_pagesize_bytes()
	{
		return sysconf(_SC_PAGESIZE);
	}

	size_t system_processor_count()
	{
		return sysconf(_SC_NPROCESSORS_ONLN);
	}

	double system_uptime_seconds()
	{
		// kernel 2.6+ compatible
		struct sysinfo system_info;
		assert(0 == sysinfo(&system_info));
		return system_info.uptime;
	}

	core::StackString<64> system_version_string()
	{
		struct utsname name;
		assert(0 == uname(&name));

		core::StackString<64> version = name.sysname;
		version.append(" ");
		version.append(name.release);

		return version;
	}

} // namespace platform


#include <platform/platform.h>
#include <platform/input.h>

#include <runtime/filesystem.h>


#include <core/core.h>
#include <core/logging.h>
#include <core/typedefs.h>
#include <core/fixedarray.h>
#include <core/stackstring.h>
#include <core/interpolation.h>
#include <core/argumentparser.h>
#include <core/mem.h>
#include <core/stackstring.h>
#include <core/datastream.h>

#include <json/json.h>


#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <vector>
#include <algorithm>
#include <thread>

using namespace std;

#if defined(PLATFORM_POSIX)
	#include <unistd.h>
#endif


#if defined(PLATFORM_APPLE)
	#include <sys/sysctl.h> // for top level identifiers, second level kernel and hw identifiers, user level identifiers
	#include <sys/gmon.h> // for third level profiling identifiers
	#include <mach/vm_param.h> // second level virtual memory identifiers
	#include <mach/host_info.h>
	#include <mach/kern_return.h>
	#include <mach/mach_host.h>
#elif defined(PLATFORM_LINUX)
	#include <sys/utsname.h>
	#include <sys/sysinfo.h>
#endif
//
//int64_t MemoryUsage()
//{
//    task_basic_info         info;
//    kern_return_t           rval = 0;
//    mach_port_t             task = mach_task_self();
//    mach_msg_type_number_t  tcnt = TASK_BASIC_INFO_COUNT;
//    task_info_t             tptr = (task_info_t) &info;
//
//    memset(&info, 0, sizeof(info));
//
//    rval = task_info(task, TASK_BASIC_INFO, tptr, &tcnt);
//    if (!(rval == KERN_SUCCESS)) return 0;
//
//    return info.resident_size;
//}


#include <platform/platform.h>
#include <core/mem.h>

struct BaseClass
{
	int xyzw;

	BaseClass()
	{
		fprintf(stdout, "BaseClass constructor\n");
	}

	virtual ~BaseClass()
	{
		fprintf(stdout, "BaseClass destructor\n");
	}
};

struct DerivedClass : public BaseClass
{
	int mytest;
	glm::quat rotation;
	glm::vec3 position;
	glm::vec3 scale;

	DerivedClass(int i = 0) :
		mytest(i)
	{
		fprintf(stdout, "DerivedClass constructor\n");
	}

	virtual ~DerivedClass()
	{
		fprintf(stdout, "DerivedClass destructor\n");
	}

	void do_work()
	{
		position += glm::vec3(1, 2, 3);
		scale = glm::vec3(1, 1, 1);
		scale *= 3.0f;

		glm::quat r(1, 0, 0, 1);
		rotation *= r;

		glm::vec3 a(1, 0, 0);
		glm::dot(a, position);
	}

	void show_value()
	{
		fprintf(stdout, "value is: %i\n", mytest);
	}
};


using namespace core::memory;

void test_memory()
{
#if 0
	int* a = MEMORY_NEW(int, global_allocator());
	BaseClass* foo = MEMORY_NEW(DerivedClass, global_allocator());
	BaseClass* values = MEMORY_NEW_ARRAY(DerivedClass, 1024, global_allocator());
	DerivedClass* derived = MEMORY_NEW_ARRAY(DerivedClass, 512, global_allocator());

	uint64_t* sixyfours = MEMORY_NEW_ARRAY(uint64_t, 16384, global_allocator());

	MEMORY_DELETE(a, global_allocator());
	MEMORY_DELETE(foo, global_allocator());
	MEMORY_DELETE_ARRAY(values, global_allocator());
	MEMORY_DELETE_ARRAY(sixyfours, global_allocator());
	MEMORY_DELETE_ARRAY(derived, global_allocator());
#endif
}

void test_maths()
{
	glm::vec3 direction = glm::normalize(glm::vec3(1, 0, 1));
	LOGV("direction: %2.2f, %2.2f, %2.2f\n", direction.x, direction.y, direction.z);

	float t = atan2(direction.z, direction.x);
	LOGV("t = %2.2f\n", t);


	// adjust
	t -= mathlib::degrees_to_radians(90);

	LOGV("angle is: %2.2f\n", mathlib::radians_to_degrees(t));

}

#include <platform/platform.h>


#if defined(PLATFORM_LINUX)

//#include <linux/input.h>
//#include <libevdev/libevdev.h>
#include <fcntl.h>


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

#include <linux/input.h>


namespace test
{
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
		virtual core::util::MemoryStream& get_stream() = 0;
	};

	InputDevice::~InputDevice()
	{
	}

	const size_t DEVICE_BUFFER_SIZE = sizeof(struct input_event) * 64;
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

			memset(buffer, 0, DEVICE_BUFFER_SIZE);
			stream.init(buffer, DEVICE_BUFFER_SIZE);
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

		virtual core::util::MemoryStream& get_stream() { return stream; }

	protected:
		int descriptor;
		core::StackString<32> device_path;
		unsigned char buffer[DEVICE_BUFFER_SIZE];
		core::util::MemoryStream stream;
	};

	const size_t MAX_KEYBOARD_KEYS = 250;

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
				printf("EVN_SYN\n");
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


	static Array<InputDevice*> _devices;

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
		_devices.push_back(device);
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
				LOGV("Unable to get device node path.\n");
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
				LOGV("Unable to find parent usb device\n");
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

			// register this device type before we unref devices
			// as they take the device_node_path with them.
			create_device_with_path(device_node_path, device_type);

			// free resources
			//udev_device_unref(uparent); // this causes a crash; do we need
			// to free the parent here?
			udev_device_unref(udevice);
		}
		udev_enumerate_unref(enumerate);
		udev_unref(lib);
	}

	bool startup()
	{
		// scan for and add devices of known types
		//
		//add_devices("input", "ID_INPUT_KEYBOARD", "1", DeviceType::Keyboard);
		// add_devices("input", "ID_INPUT_MOUSE", "1", DeviceType::Mouse);
		add_devices("input", "ID_INPUT_JOYSTICK", "1", DeviceType::Joystick);

		return true;
	}

	void shutdown()
	{
		for (InputDevice* device : _devices)
		{
			MEMORY_DELETE(device, core::memory::global_allocator());
		}
		_devices.clear();
	}

	void update()
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);

		int highest_fd = 0;

		for (InputDevice* device : _devices)
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
			for (InputDevice* device : _devices)
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
#if 0

						// struct input_event event;
						core::util::MemoryStream& stream = device->get_stream();
						size_t offset = stream.current_offset();
						if ((read_bytes+offset) >= 16)
						{
							stream.write(&buffer[0], read_bytes);
							struct input_event* event = reinterpret_cast<struct input_event*>(
								stream.get_data());

							// LOGV("interpret %i bytes\n",
							// stream.current_offset());
							if (event)
							{
								device->process_event(*event);
								stream.rewind();
							}
						}
#endif
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
}

void test_devices()
{
	test::startup();

	size_t x = 1000;
	while (true)
	{
		test::update();
		x--;
	}

	test::shutdown();
}

#endif



// test out reading BNO055.

struct DataInput
{
	bool execute;

	glm::quat orientation;
	platform::Serial* device;
	platform::Thread* thread_data;
//	EventQueueType* event_queue;
};

// raw data read from BNO055;
// quaternion data is 16-bit x 4 values
const size_t TOTAL_SENSORS = 20;

struct bno055_packet_t
{
	uint8_t header;
	uint8_t data[8 * TOTAL_SENSORS];
	uint8_t footer;

	bno055_packet_t()
	{
		header = 0xba;
		footer = 0xff;
	}

	bool is_valid() const
	{
		return (header == 0xba) && (footer == 0xff);
	}

	glm::quat get_orientation(uint32_t sensor_index = 0) const
	{
		int16_t x = 0;
		int16_t y = 0;
		int16_t z = 0;
		int16_t w = 0;

		const uint8_t* buffer = (data + (sensor_index * 8));

		// they are 16-bit LSB
		x = (((uint16_t)buffer[3]) << 8) | ((uint16_t)buffer[2]);
		y = (((uint16_t)buffer[5]) << 8) | ((uint16_t)buffer[4]);
		z = (((uint16_t)buffer[7]) << 8) | ((uint16_t)buffer[6]);
		w = (((uint16_t)buffer[1]) << 8) | ((uint16_t)buffer[0]);

		const double QUANTIZE = (1.0 / 16384.0);

		return glm::quat(w * QUANTIZE, x * QUANTIZE, y * QUANTIZE, z * QUANTIZE);
	}
};

void data_thread(platform::Thread* thread)
{
	DataInput* block = static_cast<DataInput*>(thread->user_data);

	LOGV("entering data thread\n");

	const size_t PACKET_SIZE = sizeof(bno055_packet_t);
	const size_t MAX_PACKET_DATA = 4 * PACKET_SIZE;
	uint8_t buffer[MAX_PACKET_DATA];
	size_t current_index = 0;

	while(block->execute)
	{
		size_t bytes_read = platform::serial_read(block->device, &buffer[current_index], PACKET_SIZE);
		if (bytes_read > 0)
		{
//			LOGV("read bytes: %i\n", bytes_read);
			size_t last_index = current_index;
			current_index += bytes_read;

			// scan for a valid packet
			assert(last_index < (MAX_PACKET_DATA - sizeof(bno055_packet_t)));
			uint8_t* head = &buffer[last_index];

			// try to search the entire length of a packet
			for (size_t index = 0; index < sizeof(bno055_packet_t); ++index)
			{
				bno055_packet_t* packet = reinterpret_cast<bno055_packet_t*>(head);
				if (packet->is_valid())
				{
//					gemini::GameMessage message;
//					message.type = gemini::GameMessage::Orientation;

					glm::quat q = packet->get_orientation(0);
					// this bit of code converts the coordinate system of the BNO055
					// to that of gemini.
//					glm::quat flipped(q.w, q.x, -q.z, -q.y);
//					glm::quat y = glm::quat(glm::vec3(0, mathlib::degrees_to_radians(180), 0));
//					glm::quat result = glm::inverse(y * flipped);
					LOGV("q[0]: %2.2f, %2.2f, %2.2f, %2.2f\n", q.x, q.y, q.z, q.w);

					q = packet->get_orientation(1);
					LOGV("q[1]: %2.2f, %2.2f, %2.2f, %2.2f\n", q.x, q.y, q.z, q.w);

//					block->event_queue->push_back(message);

					// handle the packet and reset the data

					memset(buffer, 0, MAX_PACKET_DATA);

					current_index = 0;
					break;
				}
			}
		}
		else
		{
			LOGV("no bytes read!\n");
		}
	}

	LOGV("exiting data thread\n");
}

void test_bno055()
{
	DataInput data_input;
#if defined(PLATFORM_APPLE) || defined(PLATFORM_LINUX)
	const char* serial_device = "/dev/cu.usbserial-AH02QPX7";
#elif defined(PLATFORM_WINDOWS)
	const char* serial_device = "COM3";
#endif
	data_input.device = platform::serial_open(serial_device, 115200);
	if (data_input.device)
	{
		assert(data_input.device != nullptr);
		data_input.execute = 1;

		data_input.thread_data = platform::thread_create(data_thread, &data_input);

		LOGV("reading data...\n");

		while (true)
		{
			// process!
		}
	}
	else
	{
		LOGE("Could not open device\n");
	}
}

int main(int, char**)
{
	gemini::core_startup();

//	test_memory();
//	test_maths();
//	test_coroutines();
//	test_serialization();
//	test_reflection();

#if defined(PLATFORM_LINUX)
	const size_t event_size = sizeof(struct input_event);
	LOGV("event_size: %i\n", event_size);

	test_devices();
#endif

	test_bno055();

	gemini::core_shutdown();

	return 0;
}


// input planning
#if 0

struct BaseClass
{
	int xyzw;

	BaseClass()
	{
		LOGV("BaseClass constructor\n");
	}

	virtual ~BaseClass()
	{
		LOGV("BaseClass destructor\n");
	}
};

struct DerivedClass : public BaseClass
{
	int mytest;

	DerivedClass()
	{
		LOGV("DerivedClass constructor\n");
	}

	virtual ~DerivedClass()
	{
		LOGV("DerivedClass destructor\n");
	}

	void move_left(float value)
	{
		LOGV("move left: %g\n", value);
	}

	void move_right(float value)
	{
		LOGV("move right: %g\n", value);
	}
};


typedef std::function<void (float)> AxisCallback;

struct InputManager
{
	core::HashSet<std::string, AxisCallback> mapping;


	template<class F, class C>
	void bind_axis(const char* name, F&& x, C&& inst)
	{
		AxisCallback callback = std::bind(x, inst, std::placeholders::_1);
		bind_axis_name(name, callback);
	}

	void bind_axis_name(const char* name, AxisCallback callback)
	{
		mapping[std::string(name)] = callback;
	}

	void dispatch(const char* name, float value)
	{
		if (mapping.has_key(name))
		{
			AxisCallback callback = mapping.get(name);
			callback(value);
		}
	}
};


//	InputManager im;
//
//	DerivedClass dc;
//
//	im.bind_axis("move_left", &DerivedClass::move_left, &dc);
//	im.bind_axis("move_right", &DerivedClass::move_right, &dc);
//
//	AxisCallback cp = std::bind(&DerivedClass::move_left, &dc, std::placeholders::_1);
//	cp(25.0f);
//
//	AxisCallback moveright = std::bind(&DerivedClass::move_right, &dc, std::placeholders::_1);
//	moveright(12.0f);
//
//	im.dispatch("move_left", 21.72f);
//	im.dispatch("move_right", 4928.0f);
#endif



// thread test
#if 0
struct TestData
{
	unsigned int value;

	TestData() : value(0)
	{
	}
};



void run_logic(void* thread_data)
{
	TestData* td = static_cast<TestData*>(thread_data);
	++td->value;

	fprintf(stdout, "test data value: %u\n", td->value);
}

void test_threads()
{
	int num_cores = 1;

#if defined(PLATFORM_POSIX)
	int page_size = sysconf(_SC_PAGE_SIZE);
	num_cores = sysconf(_SC_NPROCESSORS_CONF);
	fprintf(stdout, "page_size: %i bytes\n", page_size);
	fprintf(stdout, "cores: %i\n", num_cores);
#endif


	assert(num_cores > 0);


	platform::Thread threads[4];
	TestData test_data;

	for (uint32_t c = 0; c < num_cores; ++c)
	{
		platform::thread_create(threads[c], run_logic, &test_data);
		fprintf(stdout, "starting thread for core: %i, thread_id: %zu\n", c, threads[c].thread_id);
	}


	for (uint32_t c = 0; c < num_cores; ++c)
	{
		fprintf(stdout, "waiting on thread thread: %i\n", c);
		platform::thread_join(threads[c]);
	}
}
#endif


#if 0
void unit_test_color()
{
	unsigned int color1 = duRGBA(255, 0, 0, 0);
	unsigned int color2 = duRGBA(0, 0, 255, 0);
	unsigned int color3 = duRGBA(0, 0, 0, 255);


	Color a = Color::from_int(color1);
	Color b = Color::from_int(color2);
	Color c = Color::from_int(color3);

	assert(a.r == 255);
	assert(a.g == 0);
	assert(a.b == 0);
	assert(a.a == 0);

	assert(b.r == 0);
	assert(b.g == 0);
	assert(b.b == 255);
	assert(b.a == 0);

	assert(c.r == 0);
	assert(c.g == 0);
	assert(c.b == 0);
	assert(c.a == 255);
}
#endif


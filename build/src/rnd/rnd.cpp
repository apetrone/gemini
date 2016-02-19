#include <platform/platform.h>
#include <platform/input.h>

#include <runtime/filesystem.h>
#include <runtime/runtime.h>

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

#include "common.h"

template< typename T>
void fill( vector<int>& v, T done )
{
	int i = 0;
	while(!done())
	{
		v.push_back(i++);
	}
}



void test_function()
{
//	[](){ printf("hello\n"); }();

	vector<int> v;

	// capture by reference
	fill(v, [&](){ return v.size() >= 8; } );


	fill(v,
		 [&](){ int sum = 0;
			 for_each(begin(v), end(v), [&](int i){ sum+= 1; });
			 return sum >= 10;
		 }
	 );

	int loop = 0;
	for_each(begin(v), end(v), [&](int i){
		printf("%i -> %i\n", loop++, i);
	});
}

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


#define REFLECTION_BEGIN(class_name) static ClassProperty* reflection_fields(size_t& total_properties)\
	{\
		typedef class_name reflection_class_type;\
		static ClassProperty properties[] = {

#define REFLECTION_END() };\
	static size_t property_count = sizeof(properties)/sizeof(ClassProperty);\
	total_properties = property_count;\
	return properties;\
	}

#define REFLECTION_PROPERTY(property) ClassProperty(#property, offsetof(reflection_class_type, property))



template <class T>
struct TypeInfo
{

};

struct ClassProperty
{
	const char* name;

	size_t offset;

	ClassProperty(const char* property_name, size_t property_offset) :
		name(property_name),
		offset(property_offset)
	{
	}
};


namespace serialization
{
	template <class T>
	class Archive
	{
	public:
		template <class X>
		void serialize(X* object)
		{
			object->serialize(*this, 0);
		}
	};


	class TestArchive : public Archive<TestArchive>
	{
	public:
		template <class T>
		void serialize(T* object)
		{
			object->serialize(*this, 0);
		}
	};


	template <>
	void TestArchive::serialize(unsigned int* value)
	{
		fprintf(stdout, "serialize: unsigned int %p\n", value);
	}

	template <>
	void TestArchive::serialize(float* value)
	{
		fprintf(stdout, "serialize: float %p\n", value);
	}
}


struct TestObject
{
	uint32_t type;
	float weight;

	REFLECTION_BEGIN(TestObject)
			REFLECTION_PROPERTY(type),
			REFLECTION_PROPERTY(weight)
	REFLECTION_END()

	template <class Archive>
	void serialize(Archive& a, size_t version)
	{
		a.serialize(&type);
		a.serialize(&weight);
	}
};



void test_serialization()
{
	TestObject object;
	object.type = 30;

	serialization::TestArchive a;
	a.serialize(&object);
}

void test_reflection()
{
	size_t total_properties = 0;
	ClassProperty* properties = 0;


	properties = TestObject::reflection_fields(total_properties);
	for (size_t index = 0; index < total_properties; ++index)
	{
		ClassProperty* p = &properties[index];
		fprintf(stdout, "property: %zu, name: '%s', offset: %zu\n",
			index,
			p->name,
			p->offset);
	}
}


struct Rectangle
{
	unsigned width;
	unsigned height;
};


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

	const size_t DEVICE_BUFFER_SIZE = sizeof(struct input_event) * 2;
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

			if (descriptor == -1)
			{
				LOGW("%s (%i), Unable to open device '%s'\n",
					strerror(errno),
					errno,
					device_path_node
				);
			}
			assert(descriptor != -1);

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

			keymap[KEY_ESC] = input::BUTTON_ESCAPE;

			keymap[KEY_1] = input::BUTTON_1;
			keymap[KEY_2] = input::BUTTON_2;
			keymap[KEY_3] = input::BUTTON_3;
			keymap[KEY_4] = input::BUTTON_4;
			keymap[KEY_5] = input::BUTTON_5;
			keymap[KEY_6] = input::BUTTON_6;
			keymap[KEY_7] = input::BUTTON_7;
			keymap[KEY_8] = input::BUTTON_8;
			keymap[KEY_9] = input::BUTTON_9;
			keymap[KEY_0] = input::BUTTON_0;
			keymap[KEY_MINUS] = input::BUTTON_MINUS;
			keymap[KEY_EQUAL] = input::BUTTON_EQUALS;
			keymap[KEY_BACKSPACE] = input::BUTTON_BACKSPACE;
			keymap[KEY_TAB] = input::BUTTON_TAB;
			keymap[KEY_Q] = input::BUTTON_Q;
			keymap[KEY_W] = input::BUTTON_W;
			keymap[KEY_E] = input::BUTTON_E;
			keymap[KEY_R] = input::BUTTON_R;
			keymap[KEY_T] = input::BUTTON_T;
			keymap[KEY_Y] = input::BUTTON_Y;
			keymap[KEY_U] = input::BUTTON_U;
			keymap[KEY_I] = input::BUTTON_I;
			keymap[KEY_O] = input::BUTTON_O;
			keymap[KEY_P] = input::BUTTON_P;
			keymap[KEY_LEFTBRACE] = input::BUTTON_LBRACKET;
			keymap[KEY_RIGHTBRACE] = input::BUTTON_RBRACKET;
			keymap[KEY_ENTER] = input::BUTTON_RETURN;
			keymap[KEY_LEFTCTRL] = input::BUTTON_LCONTROL;
			keymap[KEY_A] = input::BUTTON_A;
			keymap[KEY_S] = input::BUTTON_S;
			keymap[KEY_D] = input::BUTTON_D;
			keymap[KEY_F] = input::BUTTON_F;
			keymap[KEY_G] = input::BUTTON_G;
			keymap[KEY_H] = input::BUTTON_H;
			keymap[KEY_J] = input::BUTTON_J;
			keymap[KEY_K] = input::BUTTON_K;
			keymap[KEY_L] = input::BUTTON_L;
			keymap[KEY_SEMICOLON] = input::BUTTON_SEMICOLON;
			keymap[KEY_APOSTROPHE] = input::BUTTON_TILDE;
			keymap[KEY_GRAVE] = input::BUTTON_INVALID;
			keymap[KEY_LEFTSHIFT] = input::BUTTON_LSHIFT;
			keymap[KEY_BACKSLASH] = input::BUTTON_RSHIFT;
			keymap[KEY_Z] = input::BUTTON_Z;
			keymap[KEY_X] = input::BUTTON_X;
			keymap[KEY_C] = input::BUTTON_C;
			keymap[KEY_V] = input::BUTTON_V;
			keymap[KEY_B] = input::BUTTON_B;
			keymap[KEY_N] = input::BUTTON_N;
			keymap[KEY_M] = input::BUTTON_M;
			keymap[KEY_COMMA] = input::BUTTON_COMMA;
			keymap[KEY_DOT] = input::BUTTON_PERIOD;
			keymap[KEY_SLASH] = input::BUTTON_SLASH;
			keymap[KEY_RIGHTSHIFT] = input::BUTTON_RSHIFT;
			keymap[KEY_KPASTERISK] = input::BUTTON_NUMPAD_MULTIPLY;
			keymap[KEY_LEFTALT] = input::BUTTON_LALT;
			keymap[KEY_SPACE] = input::BUTTON_SPACE;
			keymap[KEY_CAPSLOCK] = input::BUTTON_CAPSLOCK;
			keymap[KEY_F1] = input::BUTTON_F1;
			keymap[KEY_F2] = input::BUTTON_F2;
			keymap[KEY_F3] = input::BUTTON_F3;
			keymap[KEY_F4] = input::BUTTON_F4;
			keymap[KEY_F5] = input::BUTTON_F5;
			keymap[KEY_F6] = input::BUTTON_F6;
			keymap[KEY_F7] = input::BUTTON_F7;
			keymap[KEY_F8] = input::BUTTON_F8;
			keymap[KEY_F9] = input::BUTTON_F9;
			keymap[KEY_F10] = input::BUTTON_F10;
			keymap[KEY_NUMLOCK] = input::BUTTON_NUMLOCK;
			keymap[KEY_SCROLLLOCK] = input::BUTTON_SCROLLLOCK;
			keymap[KEY_KP7] = input::BUTTON_NUMPAD7;
			keymap[KEY_KP8] = input::BUTTON_NUMPAD8;
			keymap[KEY_KP9] = input::BUTTON_NUMPAD9;
			keymap[KEY_KPMINUS] = input::BUTTON_NUMPAD_MINUS;
			keymap[KEY_KP4] = input::BUTTON_NUMPAD4;
			keymap[KEY_KP5] = input::BUTTON_NUMPAD5;
			keymap[KEY_KP6] = input::BUTTON_NUMPAD6;
			keymap[KEY_KPPLUS] = input::BUTTON_NUMPAD_PLUS;
			keymap[KEY_KP1] = input::BUTTON_NUMPAD1;
			keymap[KEY_KP2] = input::BUTTON_NUMPAD2;
			keymap[KEY_KP3] = input::BUTTON_NUMPAD3;
			keymap[KEY_KP0] = input::BUTTON_NUMPAD0;
			keymap[KEY_KPDOT] = input::BUTTON_NUMPAD_PERIOD;

			keymap[KEY_ZENKAKUHANKAKU] = input::BUTTON_INVALID;
			keymap[KEY_102ND] = -1;
			keymap[KEY_F11] = input::BUTTON_F11;
			keymap[KEY_F12] = input::BUTTON_F12;

			keymap[KEY_KPENTER] = input::BUTTON_NUMPAD_ENTER;
			keymap[KEY_RIGHTCTRL] = input::BUTTON_RCONTROL;
			keymap[KEY_KPSLASH] = input::BUTTON_NUMPAD_DIVIDE;
			keymap[KEY_RIGHTALT] = input::BUTTON_RALT;

			keymap[KEY_HOME] = input::BUTTON_HOME;
			keymap[KEY_UP] = input::BUTTON_UP;
			keymap[KEY_PAGEUP] = input::BUTTON_PAGEUP;
			keymap[KEY_LEFT] = input::BUTTON_LEFT;
			keymap[KEY_RIGHT] = input::BUTTON_RIGHT;
			keymap[KEY_END] = input::BUTTON_END;
			keymap[KEY_DOWN] = input::BUTTON_DOWN;
			keymap[KEY_PAGEDOWN] = input::BUTTON_PAGEDN;
			keymap[KEY_INSERT] = input::BUTTON_INSERT;
			keymap[KEY_DELETE] = input::BUTTON_DELETE;
			// keymap[KEY_MACRO] = input::BUTTON_MACRO;
			// keymap[KEY_MUTE] = input::BUTTON_MUTE;
			// keymap[KEY_VOLUMEDOWN] = input::BUTTON_VOLUMEDOWN;
			// keymap[KEY_VOLUMEUP] = input::BUTTON_VOLUMEUP;
			//keymap[KEY_POWER]
			keymap[KEY_KPEQUAL] = input::BUTTON_NUMPAD_EQUALS;
			keymap[KEY_KPPLUSMINUS] = input::BUTTON_NUMPAD_PLUSMINUS;
			keymap[KEY_PAUSE] = input::BUTTON_PAUSE;
			//keymap[KEY_SCALE]
			//keymap[KEY_KPCOMMA]
			//keymap[KEY_HANGEUL]
			//keymap[KEY_HANGUEL]
			//keymap[KEY_HANJA]
			//keymap[KEY_YEN]
			keymap[KEY_LEFTMETA] = input::BUTTON_LOSKEY;
			keymap[KEY_RIGHTMETA] = input::BUTTON_ROSKEY;
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
			keymap[KEY_F13] = input::BUTTON_F13;
			keymap[KEY_F14] = input::BUTTON_F14;
			keymap[KEY_F15] = input::BUTTON_F15;
			keymap[KEY_F16] = input::BUTTON_F16;
			keymap[KEY_F17] = input::BUTTON_F17;
			keymap[KEY_F18] = input::BUTTON_F18;
			keymap[KEY_F19] = input::BUTTON_F19;
			keymap[KEY_F20] = input::BUTTON_F20;
			keymap[KEY_F21] = input::BUTTON_F21;
			keymap[KEY_F22] = input::BUTTON_F22;
			keymap[KEY_F23] = input::BUTTON_F23;
			keymap[KEY_F24] = input::BUTTON_F24;
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

	class JoystickDevice : public InputDeviceBase<DeviceType::Joystick>
	{
	public:

		JoystickDevice(const char* device_path) : InputDeviceBase(device_path)
		{
			using namespace input;
			// TODO: We don't need a button map per joystick
			// make this static!
			memset(buttonmap, GAMEPAD_BUTTON_INVALID, 128);

			set_button(BTN_MISC, GAMEPAD_BUTTON_INVALID);
			set_button(BTN_A, GAMEPAD_BUTTON_A);
			set_button(BTN_B, GAMEPAD_BUTTON_B);
			set_button(BTN_C, GAMEPAD_BUTTON_C);
			set_button(BTN_X, GAMEPAD_BUTTON_X);
			set_button(BTN_Y, GAMEPAD_BUTTON_Y);
			set_button(BTN_Z, GAMEPAD_BUTTON_Z);
			set_button(BTN_THUMBL, GAMEPAD_BUTTON_LEFTSTICK);
			set_button(BTN_THUMBR, GAMEPAD_BUTTON_RIGHTSTICK);
			set_button(BTN_TL, GAMEPAD_BUTTON_LEFTSHOULDER);
			set_button(BTN_TR, GAMEPAD_BUTTON_RIGHTSHOULDER);
			set_button(BTN_TL2, GAMEPAD_BUTTON_L2);
			set_button(BTN_TR2, GAMEPAD_BUTTON_R2);
			set_button(BTN_SELECT, GAMEPAD_BUTTON_SELECT);
			set_button(BTN_START, GAMEPAD_BUTTON_START);
			set_button(BTN_MODE, GAMEPAD_BUTTON_GUIDE);
		}

		void set_button(int code, input::GamepadButton button)
		{
			buttonmap[code-BTN_MISC] = button;
		}

		inline input::GamepadButton get_button(int code)
		{
			return buttonmap[code-BTN_MISC];
		}

		virtual bool process_event(const struct input_event& event)
		{
			switch(event.type)
			{
				case EV_KEY:
				{
					const input::GamepadButton button = get_button(event.code);

					if (button > input::GAMEPAD_BUTTON_INVALID && (button < input::GAMEPAD_BUTTON_COUNT))
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
		input::GamepadButton buttonmap[128];
	};


	const size_t INPUT_BUFFER_SIZE = sizeof(struct input_event);
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
		add_devices("input", "ID_INPUT_KEYBOARD", "1", DeviceType::Keyboard);
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
						unsigned char buffer[16] = {0};
						int read_bytes = 0;
						read_bytes = read(device->get_descriptor(), buffer, 16);


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


// code for reading and writing WAVE files
namespace wav
{
	// http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
	// http://soundfile.sapp.org/doc/WaveFormat/
#define MAKE_RIFF_CODE(a) ((a[0]) << 0 | (a[1]) << 8 | (a[2]) << 16 | (a[3]) << 24)
#ifndef WAVE_FORMAT_PCM
	const size_t WAVE_FORMAT_PCM = 0x001;
	const size_t WAVE_FORMAT_IEEE_FLOAT = 0x0003;
	const size_t WAVE_FORMAT_ALAW = 0x0006;
	const size_t WAVE_FORMAT_MULAW = 0x0007;
	const size_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;
#endif

	const size_t RIFF_CHUNK_ID = MAKE_RIFF_CODE("RIFF");
	const size_t RIFF_WAVE_FORMAT = MAKE_RIFF_CODE("WAVE");
	const size_t WAVE_FORMAT_CHUNK_ID = MAKE_RIFF_CODE("fmt ");
	const size_t WAVE_DATA_CHUNK_ID = MAKE_RIFF_CODE("data");

	// riff header
	struct wave_chunk_descriptor
	{
		uint32_t chunk_id; // should be 'RIFF'
		uint32_t chunk_size;
		uint32_t format; // should be 'WAVE'

		uint32_t advance_size() const
		{
			return 8 + 4;
		}
	};

	struct wave_format_chunk
	{
		uint32_t chunk_id; // should be 'fmt '
		uint32_t chunk_size; // should be 16, 18, or 40.
		uint16_t format_code; // format tag
		uint16_t total_channels; // number of interleaved channels
		uint32_t sample_rate; // blocks per second
		uint32_t data_rate; // avg bytes per sec
		uint16_t block_align; // data block size (bytes)
		uint16_t bits_per_sample;
		uint16_t extended_size; // size of extension (0 or 22)
		uint16_t valid_bits_per_sample; // number of valid bits
		uint32_t channel_mask; // speaker position mask
		uint8_t subformat[16]; // GUID including the data format code

		uint32_t advance_size() const
		{
			return 8 + chunk_size;
		}
	};

	struct wave_data_chunk
	{
		uint32_t chunk_id; // should be 'data'
		uint32_t chunk_size; // == num_samples * num_channels * bits_per_sample / 8

		uint32_t advance_size() const
		{
			return 8;
		}
	};
} // namespace wav



void test_load_wav(Array<int16_t>& samples, const char* path)
{
	using namespace wav;

	Array<unsigned char> filecontents;
	core::filesystem::instance()->virtual_load_file(filecontents, path);

	LOGV("loaded %s\n", path);
	LOGV("file size: %i bytes\n", filecontents.size());


	unsigned char* wavedata = static_cast<unsigned char*>(&filecontents[0]);
	wave_chunk_descriptor* desc = reinterpret_cast<wave_chunk_descriptor*>(wavedata);
	if (desc->chunk_id != RIFF_CHUNK_ID)
	{
		LOGV("Is not a valid RIFF file\n");
		return;
	}

	if (desc->format != RIFF_WAVE_FORMAT)
	{
		LOGV("Is not a valid WAVE file format\n");
		return;
	}

	wave_format_chunk* format = reinterpret_cast<wave_format_chunk*>(
		reinterpret_cast<char*>(desc) + desc->advance_size()
		);
	if (format->chunk_id != WAVE_FORMAT_CHUNK_ID)
	{
		LOGV("Expected WAVE 'fmt ' chunk!\n");
		return;
	}

	// We only support PCM.
	assert(format->format_code == wav::WAVE_FORMAT_PCM);

	// Only support our sample rate of 44.1KHz.
	assert(format->sample_rate == 44100);

	// Support mono and stereo sounds.
	assert(format->total_channels == 1 || format->total_channels == 2);

	wave_data_chunk* data = reinterpret_cast<wave_data_chunk*>(
		reinterpret_cast<char*>(format) + format->advance_size()
		);
	if (data->chunk_id != WAVE_DATA_CHUNK_ID)
	{
		LOGV("Expected WAVE 'data' chunk!\n");
		return;
	}

	// this describes how long in seconds the file is.
	// data->chunk_size / format->data_rate;

	const bool has_pad_byte = ((data->chunk_size + 1) & ~1) == 0;

	const uint32_t total_samples = (data->chunk_size / format->block_align);
	const uint32_t sample_size_bytes = (format->block_align / format->total_channels);

	samples.resize(total_samples * 2);
	int16_t* sample_data = reinterpret_cast<int16_t*>(reinterpret_cast<char*>(data) + data->advance_size());
	for (size_t sample = 0; sample < total_samples * 2; ++sample)
	{
		samples[sample] = sample_data[sample];
	}
}


void test_write_wav(Array<int16_t>& samples, const char* path)
{
	FILE* f = fopen(path, "wb");

	wav::wave_chunk_descriptor desc;
	desc.chunk_id = MAKE_RIFF_CODE("RIFF");
	desc.format = MAKE_RIFF_CODE("WAVE");
	// TODO: fill out desc with the total file size - 8

	wav::wave_format_chunk format;
	format.chunk_id = MAKE_RIFF_CODE("fmt ");
	format.chunk_size = 16;
	format.format_code = wav::WAVE_FORMAT_PCM;
	format.total_channels = 2;
	format.sample_rate = 44100;
	format.bits_per_sample = 16;
	format.data_rate = format.sample_rate * format.total_channels * (format.bits_per_sample / 8);
	format.block_align = (format.total_channels * format.bits_per_sample) / 8;

	wav::wave_data_chunk data;
	data.chunk_id = MAKE_RIFF_CODE("data");
	data.chunk_size = (samples.size() * format.bits_per_sample) / 8;
	desc.chunk_size = data.chunk_size + 36;

	if (f)
	{
		fwrite(&desc, 1, desc.advance_size(), f);
		fwrite(&format, 1, format.advance_size(), f);
		fwrite(&data, 1, data.advance_size(), f);
		fwrite(&samples[0], 2, samples.size(), f);

		// see if we need to write a padding byte.
		if ((data.chunk_size + 1) & ~1 == 0)
		{
			char padding = '\0';
			fwrite(&padding, 1, 1, f);
		}
		fclose(f);
	}
}

// potential test code for reading/writing wav files
#if 0
void test_wav()
{
	Array<int16_t> samples;
	test_load_wav(samples, "sound.wav");

	test_write_wav(samples, "output.wav");

	Array<int16_t> input;
	test_load_wav(input, "output.wav");
}
#endif

// test windows audio
#if defined(PLATFORM_WINDOWS)
#include <core/logging.h>
#include <platform/platform.h> // for dylib manipulation

#pragma warning(push)
// Cleanup after messy windows headers.
#pragma warning(disable: 4917) // a GUID can only be associated with a class, interface or namespace
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <AudioSessionTypes.h>
#include <functiondiscoverykeys_devpkey.h> // for endpoint device property keys
#pragma warning(pop)

#include <objbase.h>

template <class T>
void safe_release(T** ptr)
{
	if (*ptr)
	{
		(*ptr)->Release();
		*ptr = nullptr;
	}
}

void enumerate_devices(IMMDeviceCollection* device_collection)
{
	UINT device_count;
	device_collection->GetCount(&device_count);
	for (UINT index = 0; index < device_count; ++index)
	{
		IMMDevice* device;
		if (SUCCEEDED(device_collection->Item(index, &device)))
		{
			// uuid of device
			//wchar_t* name;
			//if (SUCCEEDED(device->GetId(&name)))
			//{
			//	CoTaskMemFree(name);
			//}

			IPropertyStore* properties;
			if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &properties)))
			{
				//PROPVARIANT friendly_name;
				//PropVariantInit(&friendly_name);
				//if (SUCCEEDED(properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendly_name)))
				//{
				//	LOGV("endpoint: %i '%S'\n", index, friendly_name.pwszVal);
				//	PropVariantClear(&friendly_name);
				//}

				//PROPVARIANT device_desc;
				//PropVariantInit(&device_desc);
				//if (SUCCEEDED(properties->GetValue(PKEY_Device_DeviceDesc, &device_desc)))
				//{
				//	LOGV("endpoint: %i '%S'\n", index, device_desc.pwszVal);
				//	PropVariantClear(&device_desc);
				//}

				// combined: FriendlyName (DeviceInterface)
				PROPVARIANT device_name;
				PropVariantInit(&device_name);
				if (SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName, &device_name)))
				{
					LOGV("endpoint: %i '%S'\n", index, device_name.pwszVal);
					PropVariantClear(&device_name);
				}

				safe_release(&properties);
			}
		}
	}
}

HRESULT report_error(HRESULT input)
{
	switch (input)
	{
		// No error.
	case S_OK:
		break;

	case AUDCLNT_E_ALREADY_INITIALIZED:
		LOGV("AudioClient is already initialized\n");
		break;

	case AUDCLNT_E_BUFFER_ERROR:
		LOGV("GetBuffer failed to retrieve a data buffer\n");
		break;

	case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
		LOGV("Wrong Endpoint Type\n");
		break;

	case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
		LOGV("BufferSize is not aligned\n");
		break;

	case AUDCLNT_E_BUFFER_SIZE_ERROR:
		LOGV("Buffer duration for exclusive mode is out of range\n");
		break;

	case AUDCLNT_E_UNSUPPORTED_FORMAT:
		LOGV("Unsupported format\n");
		break;

	case AUDCLNT_E_BUFFER_TOO_LARGE:
		LOGV("NumFramesRequested value exceeds available buffer space (buffer size minus padding size)\n");
		break;

	default:
		LOGV("Unknown Error\n");
		break;
	}

	return input;
}

static uint32_t t = 0;
Array<int16_t> loaded_sound;

// sample_rate_hz == 1 second.
// 260 cycles per second
// sample_rate_hz / 260

void fill_buffer(BYTE* data, UINT32 frames_available, const UINT32 engine_sample_size, const UINT32 block_align)
{
	// frame_size = channels (two) * sample_size (two bytes)
	// frame_size is 4 bytes; multiply by frames_available is your buffer size.


	// load a wave
#if 0
	int16_t* buffer = reinterpret_cast<int16_t*>(data);
	for (UINT32 frame = 0; frame < frames_available; ++frame)
	{
		buffer[frame * 2] = loaded_sound[t];
		t += 2;
		if (t > loaded_sound.size() - 1)
			t = 0;
	}
#endif

#if 0
	// generate a square wave
	const float volume = 0.25f;

	const UINT32 frames_per_period = 44100 / 256;
	const UINT32 half_period = frames_per_period / 2;
	const UINT32 total_frames = frames_available;

	int16_t* buffer = reinterpret_cast<int16_t*>(data);
	for (UINT32 frame = 0; frame < total_frames; ++frame)
	{
		short value = ((t / half_period) % 2) ? 3000 : -3000;
		uint32_t index = frame * 2;
		buffer[index + 0] = static_cast<short>(value * volume);
		buffer[index + 1] = static_cast<short>(value * volume);

		t += 1;
		if (t > frames_per_period)
		{
			t -= frames_per_period;
		}
	}
#endif
}

#include <avrt.h>

platform::Result test_wasapi()
{
	// After a brief look at XAudio2 here's my conclusion:
	// - a. There are three different versions for Windows 7, 8, and 10.
	//		Headers that ship with Windows 8.1 kit (which can be installed
	//		on Windows 7 via VS2015, link to XAudio2_8.dll by default.
	//		This requires that the application load the XAudio2_7.dll at
	//		runtime and dynamically link the functions.
	// - b. This leads down the rabbit hole of having to duplicate what the
	//		June 2010 DX SDK headers do for XAudio2Create
	//		via COM, initializing the instance, etc.
	// - c. On top of that awful bit, XAudio2 is limited in that it cannot
	//		capture audio from devices -- such as a Microphone.
	// - d. WASAPI still leaves the door open for Windows Store and
	//		Phone Apps (in case that ever needs to be a thing).

	// Therefore, I'm going to attempt to use the WASAPI which debuted with
	// Windows Vista. If there's ever a need to support Windows XP, I can hook
	// up DirectSound later on.

	//
	// REFERENCES
	//
	// Sample rate conversion is not performed inside WASAPI.
	// Exclusive mode vs Shared mode with the device. Exclusive is a direct
	// connection -- no other applications can use the device.
	// http://mark-dot-net.blogspot.com/2008/06/what-up-with-wasapi.html

	// Need to add in a bit of latency when writing audio
	// https://hero.handmadedev.org/forum/code-discussion/442-using-wasapi-for-sound#2902

	// REFERENCE_TIME is in hundreds of nanoseconds.
	// https://blogs.windows.com/buildingapps/2014/05/15/real-time-audio-in-windows-store-and-windows-phone-apps/

	platform::Result result;

	// load a test wav.
	test_load_wav(loaded_sound, "sound.wav");


	HANDLE task_handle;
	DWORD task_index = 0;


	// We must initialize COM first. This requires objbase.h and ole32.lib.
	if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
	{
		assert(!"COM initialization failed");
		return platform::Result::failure("COM initialization failed");
	}


	// try to create the device enumerator instance.
	IMMDeviceEnumerator* device_enumerator;
	HRESULT create_enumerator_result = CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		0,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<LPVOID*>(&device_enumerator)
		);
	if (FAILED(create_enumerator_result))
	{
		return platform::Result::failure("IMMDeviceEnumerator creation failed");
	}

	// let's enumerate the audio endpoint devices.
	const EDataFlow device_type = eRender; // eRender, eCapture, eAll.
	const DWORD state_mask = DEVICE_STATE_ACTIVE;
	IMMDeviceCollection* device_collection;
	HRESULT enum_result = device_enumerator->EnumAudioEndpoints(device_type, state_mask, &device_collection);
	if (FAILED(enum_result))
	{
		return platform::Result::failure("Failed to enumerate devices");
	}
	else
	{
		//enumerate_devices(device_collection);
		safe_release(&device_collection);
	}

	IMMDevice* default_render_device;
	if (FAILED(device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &default_render_device)))
	{
		return platform::Result::failure("Get default render device failed");
	}

	IMMDevice* default_capture_device;
	if (FAILED(device_enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &default_capture_device)))
	{
		return platform::Result::failure("Get default capture device failed");
	}

	IAudioClient* audio_client;
	// try to activate the render device.
	if (FAILED(default_render_device->Activate(__uuidof(IAudioClient),
		CLSCTX_ALL,
		0,
		reinterpret_cast<LPVOID*>(&audio_client))))
	{
		return platform::Result::failure("Output device Activation failed");
	}


	WAVEFORMATEX* mix_format;
	audio_client->GetMixFormat(&mix_format);

	const UINT32 mix_sample_size = mix_format->nBlockAlign;



	const DWORD sample_rate_hz = 44100;  // 44.1KHz
	assert(mix_format->nSamplesPerSec == sample_rate_hz);
	WAVEFORMATEX waveformat;
	memset(&waveformat, 0, sizeof(WAVEFORMATEX));
	waveformat.nChannels = 2;
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.nSamplesPerSec = sample_rate_hz;
	waveformat.wBitsPerSample = 16;
	waveformat.nBlockAlign = (waveformat.nChannels * waveformat.wBitsPerSample) / 8;
	waveformat.nAvgBytesPerSec = (waveformat.nSamplesPerSec * waveformat.nBlockAlign);
	waveformat.cbSize = 0;

	const UINT32 block_align = mix_sample_size * waveformat.nBlockAlign;

	//
	// Check to see if the format is supported in a certain share mode.
	//
	//WAVEFORMATEX* shared_format;
	//HRESULT format_supported = audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &waveformat, &shared_format);
	//if (SUCCEEDED(format_supported))
	//{
	//	LOGV("format IS supported. setting...\n");
	//	CoTaskMemFree(shared_format);
	//}
	//else
	//{
	//	// format is NOT supported for EXCLUSIVE mode.
	//	assert(0);
	//}

	//
	// Initialize an audio stream
	//

	REFERENCE_TIME default_device_period;
	REFERENCE_TIME min_device_period;
	assert(S_OK == report_error(audio_client->GetDevicePeriod(&default_device_period, &min_device_period)));

	//double period_seconds = (default_device_period / 1.0e7);
	//const UINT32 frames_per_period = static_cast<UINT32>(sample_rate_hz * period_seconds + 0.5);

	const double BUFFER_SECONDS = 1.0;
	//REFERENCE_TIME buffer_duration = static_cast<REFERENCE_TIME>(BUFFER_SECONDS * static_cast<double>(4096) / (sample_rate_hz * 1.0e-7));
	REFERENCE_TIME buffer_duration = 1.0e7;


	const DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	HRESULT stream_initialize = audio_client->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		stream_flags,
		buffer_duration,
		0, // always zero in SHARED mode.
		&waveformat,
		NULL
		);
	assert(report_error(stream_initialize) == S_OK);

	// create the event and set it on the audio client
	HANDLE event_handle = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		NULL
		);

	HRESULT set_event_result = audio_client->SetEventHandle(event_handle);
	if (set_event_result != S_OK)
	{
		return platform::Result::failure("failed to set the audio client event handle");
	}

	UINT32 buffer_frame_count;
	HRESULT get_buffer_size_result = audio_client->GetBufferSize(&buffer_frame_count);
	if (get_buffer_size_result != S_OK)
	{
		return platform::Result::failure("Failed to GetBufferSize");
	}

	LOGV("buffer_frame_count = %i\n", buffer_frame_count);

	REFERENCE_TIME actual_duration = static_cast<double>(
		1.0e7 * (buffer_frame_count / static_cast<double>(mix_format->nSamplesPerSec))
		);


	// clean up mix format
	CoTaskMemFree(mix_format);


	IAudioRenderClient* render_client;
	HRESULT get_service_result = audio_client->GetService(
		__uuidof(IAudioRenderClient),
		reinterpret_cast<LPVOID*>(&render_client)
		);
	if (get_service_result != S_OK)
	{
		return platform::Result::failure("Failed to GetService: IAudioRenderClient");
	}


	task_handle = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &task_index);
	assert(task_handle != NULL);

	LOGV("setup thread for task: %i\n", task_index);

	BYTE* buffer_data;
	HRESULT get_buffer_result = render_client->GetBuffer(
		buffer_frame_count,
		&buffer_data
		);
	if (get_buffer_result != S_OK)
	{
		return platform::Result::failure("Failed to get initial buffer data");
	}

	fill_buffer(buffer_data, buffer_frame_count, mix_sample_size, block_align);

	const DWORD release_flags = 0;
	HRESULT release_buffer_result = render_client->ReleaseBuffer(
		buffer_frame_count,
		release_flags
		);
	if (release_buffer_result != S_OK)
	{
		return platform::Result::failure("Unable to release initial buffer");
	}

	// https://msdn.microsoft.com/en-us/library/windows/desktop/dd316756(v=vs.85).aspx

	HRESULT start_result = audio_client->Start();
	assert(start_result == S_OK);

	size_t iterations = 300;

	LOGV("begin sound processing...\n");

	for (size_t iter = 0; iter < iterations; ++iter)
	{
		// block until the next audio buffer is signaled
		WaitForSingleObject(event_handle, INFINITE);

		UINT32 padding_frames = 0;
		UINT32 frames_available = 0;

		// get padding in existing buffer
		assert(S_OK == audio_client->GetCurrentPadding(&padding_frames));

		BYTE* new_buffer;

		// get available frames
		frames_available = buffer_frame_count - padding_frames;
		get_buffer_result = render_client->GetBuffer(
			frames_available,
			&new_buffer
			);
		assert(report_error(get_buffer_result) == S_OK);

		// fill the buffer
		fill_buffer(new_buffer, frames_available, mix_sample_size, block_align);

		release_buffer_result = render_client->ReleaseBuffer(frames_available, 0);
		assert(report_error(release_buffer_result) == S_OK);
	}

	LOGV("finished sound processing...\n");

	audio_client->Stop();

	safe_release(&render_client);

	safe_release(&audio_client);
	if (event_handle)
	{
		CloseHandle(event_handle);
	}

	AvRevertMmThreadCharacteristics(task_handle);

	// shutdown
	safe_release(&default_capture_device);
	safe_release(&default_render_device);
	safe_release(&device_enumerator);


	CoUninitialize();


	return platform::Result::success();
}
#endif





int main(int argc, char** argv)
{
	gemini::core_startup();
	gemini::runtime_startup("arcfusion.net/gemini/rnd");

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

#if defined(PLATFORM_WINDOWS)
	platform::Result test_wasapi();
	platform::Result test = test_wasapi();
	assert(test.succeeded());
#endif

	gemini::runtime_shutdown();
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


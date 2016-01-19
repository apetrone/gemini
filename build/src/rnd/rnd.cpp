#include <platform/platform.h>
#include <core/typedefs.h>

#include <runtime/core.h>
#include <runtime/filesystem.h>
#include <runtime/logging.h>

#include <core/fixedarray.h>
#include <core/stackstring.h>
#include <core/interpolation.h>
#include <core/argumentparser.h>
#include <core/mem.h>

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

void test_udev(const char* subsystem)
{
	// http://www.signal11.us/oss/udev/
	using namespace platform;
	struct udev* lib;
	struct udev_enumerate* enumerate;
	struct udev_list_entry* devices;
	struct udev_list_entry* entry;
	struct udev_device* dev;

	// create
	lib = udev_new();
	if (!lib)
	{
		fprintf(stderr, "cannot create the udev lib\n");
		return;
	}

	// create a list of devices in the 'hidraw' subsystem
	enumerate = udev_enumerate_new(lib);

	fprintf(stdout, "looking for subsystem matches: '%s'\n", subsystem);
	udev_enumerate_add_match_subsystem(enumerate, subsystem);
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(entry, devices)
	{
		const char* path;
		// get the filename of the /sys entry for the device
		// and create a udev_device object representing it.

		path = udev_list_entry_get_name(entry);
		PLATFORM_LOG(LogMessageType::Info, "entry name: %s\n",
			path);

		dev = udev_device_new_from_syspath(lib, path);
		if (!dev)
		{
			PLATFORM_LOG(LogMessageType::Warning, "Unable to find device"
				" for sys path: %s\n", path);
			continue;
		}
		// usb_device_get_devnode() returns the path to the device node
		// in /dev/
		PLATFORM_LOG(LogMessageType::Info, "Device Node Path: %s\n",
			udev_device_get_devnode(dev));


		// The device pointed to by dev contains info about the hidraw
		// device. In order to get info about the device, get the
		// parent usb device with the subsystem/devtype pair of
		// "usb"/"usb_device". This will be several levels up the
		// tree, but the function will find it.
		dev = udev_device_get_parent_with_subsystem_devtype(
			dev,
			"usb",
			"usb_device");

		if (!dev)
		{
			PLATFORM_LOG(LogMessageType::Info,
				"Unable to find parent usb device\n");
			return;
		}

		// from here, call get_sysattr_value for each file in the
		// device's /sys entry. The strings passed into these
		// functions (idProduct, idVendor, serial, etc.) correspond
		// directly to the files in the directory representing the
		// usb device. Note that USB strings are Unicode, UCS2
		// encoded, but the strings returned from
		// udev_device_get_sysattr_value are UTF-8 encoded.
		PLATFORM_LOG(LogMessageType::Info, "VID/PID: %s %s\n",
			udev_device_get_sysattr_value(dev, "idVendor"),
			udev_device_get_sysattr_value(dev, "idProduct"));

		PLATFORM_LOG(LogMessageType::Info, "%s\n %s\n",
			udev_device_get_sysattr_value(dev, "manufacturer"),
			udev_device_get_sysattr_value(dev, "product"));

		PLATFORM_LOG(LogMessageType::Info, "%s\n",
			udev_device_get_sysattr_value(dev, "serial"));

		udev_device_unref(dev);
	}
	udev_enumerate_unref(enumerate);
	udev_unref(lib);
}
#endif

//idVendor:idProduct
// 045e:028e -- Xbox360 controller

// etc/udev/rules.d/99-input.rules
// KERNEL=="event[0-9]*", ENV{ID_BUS}=="?*", ENV{ID_INPUT_JOYSTICK}=="?*", GROUP="games", MODE="0660"
// KERNEL=="js[0-9]*", ENV{ID_BUS}=="?*", ENV{ID_INPUT_JOYSTICK}=="?*", GROUP="games", MODE="0664"

// reload rules: udevadm control --reload-rules
// udevtrigger

// https://wiki.archlinux.org/index.php/Gamepad#PlayStation_4_controller

int main(int argc, char** argv)
{
	platform::startup();

//	test_memory();
//	test_maths();
//	test_coroutines();
//	test_serialization();
//	test_reflection();

#if defined(PLATFORM_LINUX)
	if (argc == 2)
	{
		PLATFORM_LOG(platform::LogMessageType::Info,
			"Subsystem filter: %s\n", argv[1]);
		test_udev(argv[1]);
	}
#endif

//	core::shutdown();
	platform::shutdown();

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


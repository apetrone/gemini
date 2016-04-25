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

#include <platform/platform.h>
#include <platform/window.h>

int main(int, char**)
{
	gemini::core_startup();

//	test_memory();
//	test_maths();
//	test_coroutines();
//	test_serialization();
//	test_reflection();

	platform::window::startup(platform::window::RenderingBackend_Default);

	while (true)
	{
		platform::update(0.0f);
	}
	//test_bno055();

	platform::window::shutdown();

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


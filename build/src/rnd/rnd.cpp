#include <stdio.h>
#include <stdlib.h>

#include <functional>
#include <vector>
#include <algorithm>
#include <thread>

using namespace std;

#include <platform/typedefs.h>
#include <platform/mem.h>
#include <gemini/core.h>
#include <core/filesystem.h>
#include <gemini/core/log.h>
#include <core/stackstring.h>
#include <gemini/core/xfile.h>
#include <core/fixedarray.h>
#include <core/arg.h>

#include <slim/xlog.h>


#include <json/json.h>

#define USE_SDL2 1

#if USE_SDL2
	#include <SDL.h>
#endif


#if PLATFORM_MACOSX
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl.h>
#elif PLATFORM_LINUX
	#if PLATFORM_USE_GLES2
		#include <GLES2/gl2.h>
	#else
		#include <GL/gl.h>
		#include <GL/glx.h>
	#endif
#else
	#include <gl/GL.h>
#endif

#include "common.h"

//#include "keyframechannel.h"

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



#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>


#if defined(__linux__) || defined(__APPLE__)
	#include <unistd.h>
	#include <termios.h>
#elif defined(WIN32)
	#error Not implemented on this platform!
#endif

class SerialPort
{
public:

	SerialPort() : socket(-1) {}
	~SerialPort() { close(); }

	bool open(const char* serial_port, uint32_t baud_rate = 9600);
	void close();
	
	void readline(std::string& buffer);
	
	int read(void* buffer, int total_bytes);
	int write(const void* buffer, int total_bytes);
	
private:
	void baud_rate(uint32_t baud_rate);
	
private:
	int32_t socket;
};


bool SerialPort::open(const char* serial_port, uint32_t baud)
{
	socket = ::open(serial_port, O_RDWR | O_NOCTTY | O_NDELAY);
	if (socket == -1)
	{
		LOGE("Unable to open serial port %s\n", serial_port);
		return false;
	}

	// clear file status
	fcntl(socket, F_SETFL, 0);
	
	// set baud rate
	baud_rate(baud);

	return true;
}

void SerialPort::close()
{
	if (socket != -1)
	{
		::close(socket);
	}
}

void SerialPort::readline(std::string& buffer)
{
	std::string b;
	char last = 0;
	bool co = true;
	while (co)
	{
		char c = 0;
		read(&c, 1);

		// found newline
		if (c == '\n' && last == '\r')
		{
			LOGV("read: %s\n", b.c_str());
			b.clear();
			last = 0;
		}
		else if (c != '\r')
		{
			b += c;
		}

		last = c;
	}
}

int SerialPort::read(void* buffer, int total_bytes)
{
	return ::read(socket, buffer, total_bytes);
}

int SerialPort::write(const void* buffer, int total_bytes)
{
	return ::write(socket, buffer, total_bytes);
}

void SerialPort::baud_rate(uint32_t baud_rate)
{
	// setup the baud rate
	struct termios options;
	tcgetattr(socket, &options);
	
	cfsetispeed(&options, baud_rate);
	cfsetospeed(&options, baud_rate);
	
	// enable receiver and set local mode
	options.c_cflag |= (CLOCAL | CREAD);
	
	// set new options
	tcsetattr(socket, TCSANOW, &options);
}


void test_rendering()
{

#if USE_SDL2
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Window* window = 0;
//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
//	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	window = SDL_CreateWindow(
		"rnd",
		0, 0, 800, 600,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);
	
	SDL_GLContext context = SDL_GL_CreateContext(window);

				
	SDL_Event event;
	bool run = true;
	
	
	// print out GL
	LOGV("GL_VERSION: %s\n", glGetString(GL_VERSION));
	LOGV("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	LOGV("GL_VENDOR: %s\n", glGetString(GL_VENDOR));

	
	
	while(run)
	{
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					break;
			
				case SDL_KEYDOWN:
				{
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						run = false;
					}
					break;
				}
			}
		}
		
		glViewport(0, 0, 800, 600);
		glClearColor(0.25f, 0.25f, 0.35f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 800, 0, 600, -0.1f, 10.0f);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, -1.0f);
		

		glColor3ub(255, 255, 255);
		glBegin(GL_TRIANGLES);
		glVertex2d(0, 10);
		glVertex2d(50, 10);
		glVertex2d(25, 30);
		glEnd();

		
		SDL_GL_SwapWindow(window);
	}
	
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
#endif
}

#if 0
void test_animation()
{
	// animation sample rate
	const float FRAME_RATE = (1.0f/30.0f);
	
	// time between updates
	const float FRAME_DELTA = (1.0f/60.0f);
	
	float values[] = { 0.0f, 0.25f, 0.5f, 1.0f};
	KeyframeChannel<float> k1;
	k1.create(4, values, FRAME_RATE);
	
	
	KeyframeData<float> data;
	data.keys.allocate(4);
	data.keys[0] = values[0];
	data.keys[1] = values[1];
	data.keys[2] = values[2];
	data.keys[3] = values[3];
	
	data.time.allocate(4);
	data.time[0] = 0.0f;
	data.time[1] = FRAME_RATE;
	data.time[2] = FRAME_RATE * 2.0f;
	data.time[3] = FRAME_RATE * 3.0f;
	
	float value = 0.0f;
	Channel<float> c0(value);
	c0.set_data_source(&data, FRAME_RATE);
	
	
	float accum = 0;
	for(int i = 0; i < 10; ++i)
	{
		c0.update(FRAME_DELTA);
		
		LOGV("frame: %i, c0 value: %g, k1 value: %g\n", i, value, k1.get_value(accum));
		accum += FRAME_DELTA;
		if (accum >= (FRAME_DELTA*6))
		{
			accum -= (FRAME_DELTA*6);
		}
	}
}
#endif

void test_serial2()
{
//	const char port[] = "/dev/tty.usbmodem1441";
//	const uint32_t baud_rate = 9600;
	
	const char port[] = "/dev/tty.usbserial-A9014A4H";
//	const char port[] = "/dev/tty.usbserial-A100RYUU";
	const uint32_t baud_rate = 115200;
	
	SerialPort s;
	if (s.open(port, baud_rate))
	{
//		LOGV("writing data...\n");
//		for (int i = 0; i < 0xFFF; ++i)
//		{
//			const char data[] = "adam";
//			s.write(data, 4);
//		}
//		LOGV("finished writing data\n");

		LOGV("reading data...\n");


		for(int i = 0; i < 0xFFFF; ++i)
		{
//			char buffer[128] = {0};
//			int bytes = s.read(buffer, 4);
//			LOGV("read %i, %s\n", bytes, buffer);

			std::string buffer;
			s.readline(buffer);
			LOGV("%s\n", buffer.c_str());
		}
		
		LOGV("finished reading data..\n");
	}
}

int main(int argc, char** argv)
{
	tools::startup();
//	test_function();
	std::thread t0(test_serial2);
	
	
	test_rendering();
//  test_animation();
	t0.join();
	
	tools::shutdown();
	return 0;
}
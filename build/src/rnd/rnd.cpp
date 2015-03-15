#include <stdio.h>
#include <stdlib.h>

#include <functional>
#include <vector>
#include <algorithm>
#include <thread>

using namespace std;

#include <platform/platform.h>

#include <core/typedefs.h>
#include <core/core.h>
#include <core/filesystem.h>
#include <core/logging.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>
#include <core/interpolation.h>
#include <core/dictionary.h>
#include <core/argumentparser.h>

#include <json/json.h>




#define USE_SDL2 1

#if USE_SDL2
	#include <SDL.h>

	// So, on Windows, this is defined to SDL_main.
	// Don't do that.
#if PLATFORM_WINDOWS
	#undef main
#endif
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
	//#error Not implemented on this platform!
#endif
#if 0
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
#endif

struct ApplicationState
{
	float accumulator;
	float framedelta_msec;
	float time;
	
	glm::vec3 last_position;
	glm::vec3 position;
	glm::vec3 render_position;
	
	
	ApplicationState()
	{
		accumulator = 0.0f;
		framedelta_msec = 0.0f;
		time = 0.0f;
		position = last_position = glm::vec3(0.0f, 300.0f, 0.0f);
	}
};

const float STEP_INTERVAL = (1.0f/30.0f);

void run_frame(ApplicationState& state)
{
//	LOGV("df: %g\n", state.framedelta_msec);
	float delta_seconds = (state.framedelta_msec*.001f);
	state.accumulator += delta_seconds;
	state.time += delta_seconds;
//	LOGV("accumulator: %2.2f\n", state.accumulator);
	
	if (state.accumulator >= STEP_INTERVAL)
	{
		state.accumulator -= STEP_INTERVAL;
		
		state.last_position = state.position;
		state.position += glm::vec3(70.0f, 0.0f, 0.0f) * STEP_INTERVAL;
//		state.position = glm::vec3(400+(300.0f*cos(state.time)),300+(300.0f*sin(state.time)), 0.0f) * STEP_INTERVAL;
	}
	
	float alpha = state.accumulator / STEP_INTERVAL;
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}
//	LOGV("alpha is: %g\n", alpha);
	
	state.render_position = core::lerp(state.last_position, state.position, alpha);


	glViewport(0, 0, 800, 600);
	glClearColor(0.25f, 0.25f, 0.35f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 800, 0, 600, -0.1f, 10.0f);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -1.0f);
	
	
	{
		glm::vec2 tri[3];
		tri[0] = glm::vec2(state.render_position.x, state.render_position.y+10);
		tri[1] = glm::vec2(state.render_position.x+50, state.render_position.y+10);
		tri[2] = glm::vec2(state.render_position.x+25, state.render_position.y+30);
		
		glColor3ub(255, 0, 255);
		glBegin(GL_TRIANGLES);

		glVertex2fv(glm::value_ptr(tri[0]));
		glVertex2fv(glm::value_ptr(tri[1]));
		glVertex2fv(glm::value_ptr(tri[2]));
		
		glEnd();
	}
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

	ApplicationState state;
	uint64_t last_ticks = platform::instance()->get_time_microseconds();
	
	while(run)
	{
		uint64_t current_ticks = platform::instance()->get_time_microseconds();
		
		state.framedelta_msec = (current_ticks - last_ticks) * 0.001f;
		last_ticks = current_ticks;
	
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

		run_frame(state);

		
		SDL_GL_SwapWindow(window);
	}
	
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
#endif
}

/*
#include <core/dictionary.h>

void test_hash()
{
	core::Dictionary<int> d;
	
	d.insert("adam", 30);
	d.insert("what", 32);
	
	if (d.has_key("what"))
	{
		LOGV("dictionary has key\n");
	}
	else
	{
		LOGV("dictionary doesn't have key!\n");
	}

	int value = 0;
	d.get("adam", value);
	d.get("what", value);
}
*/


void test_args(int argc, char** argv)
{
	// Options can be specified in any order.
	
	// long option
	// single letter option
	
	// command, positional, optional elements, argument
	// export <position> [--first] FILE
	
	// [command --option <argument>] == [command] [--option] [<argument>]
	
	// | means mutually exclusive
	// ( ) groups elements
	
	// [options] (shorthand for all options) OR [-alh]
	
	// after [--] denotes following arguments are positional
	
	// [-] used to denote a program will take input from stdin as opposed to a file
	// Usage: my_program [-| <file>...]
	
	/*
		// anything that starts with a dash is documentation for the option
		Options:
			-- verbose  # GOOD
			-o FILE		# GOOD
			
		Other:  --bad	# BAD
	*/
	
	// [default: 2.95]
	
	// ellipses (allows additional arguments)
	
//	IEEE Std 1003.1

	core::argparse::ArgumentParser parser;
	
	// test1: usage not explicitly defined (error)
	const char* test1 = R"(
	tcp [<host>] [--force] [--timeout=<seconds>]
	serial <port> [--baud=<rate>] [--timeout=<seconds>]
	-h | --help | --version
	)";
	
	// test2: explicit usage and option sections
	const char* test2 = R"(
Usage:
	tcp <host> <port> [--baud=9600] [--timeout=<seconds>]
	serial <port> [--baud=9600] [--timeout=<seconds>]
	
Options:
	-h, --help  Display this help string
	--version   Show the program version
	)";

	// test3: added arguments section
	const char* test3 = R"(
Usage: arguments_example.py [-vqrh] [FILE] ...
	arguments_example.py (--left | --right) CORRECTION FILE
	Process FILE and optionally apply correction to either left-hand side or
	right-hand side.
Arguments:
	FILE        optional input file
	CORRECTION  correction angle, needs FILE, --left or --right to be present
Options:
	-h --help
	-v       verbose mode
	-q       quiet mode
	-r       make report
	--left   use left-hand side
	--right  use right-hand side
	)";
	
	// test4: naval fate example
	const char* test4 = R"(
Usage:
	naval_fate.py ship new <name>...
	naval_fate.py ship <name> move <x> <y> [--speed=<kn>]
	naval_fate.py ship shoot <x> <y>
	naval_fate.py mine (set|remove) <x> <y> [--moored|--drifting]
	naval_fate.py -h | --help
	naval_fate.py --version
Options:
	-h --help     Show this screen.
	--version     Show version.
	--speed=<kn>  Speed in knots [default: 10].
	--moored      Moored (anchored) mine.
	--drifting    Drifting mine.
	)";
	
	// test5: odd even example
	const char* test5 = R"(
Usage: odd_even_example.py [-h | --help] (ODD EVEN)...
	Example, try:
	odd_even_example.py 1 2 3 4
Options:
	-h, --help
	)";
	
	// test6: many options
	const char* test6 = R"(
	Example of program with many options using docopt.
Usage:
	options_example.py [-hvqrf NAME] [--exclude=PATTERNS]
	[--select=ERRORS | --ignore=ERRORS] [--show-source]
	[--statistics] [--count] [--benchmark] PATH...
	options_example.py (--doctest | --testsuite=DIR)
	options_example.py --version
Arguments:
	PATH  destination path
Options:
	-h --help            show this help message and exit
	--version            show version and exit
	-v --verbose         print status messages
	-q --quiet           report only file names
	-r --repeat          show all occurrences of the same error
	--exclude=PATTERNS   exclude files or directories which match these comma
	separated patterns [default: .svn,CVS,.bzr,.hg,.git]
	-f NAME --file=NAME  when parsing directories, only check filenames matching
	these comma separated patterns [default: *.py]
	--select=ERRORS      select errors and warnings (e.g. E,W6)
	--ignore=ERRORS      skip errors and warnings (e.g. E4,W)
	--show-source        show source code for each error
		--statistics         count errors and warnings
		--count              print total number of errors and warnings to standard
		error and set exit code to 1 if total is not null
			--benchmark          measure processing speed
			--testsuite=DIR      run regression tests from dir
			--doctest            run doctest on myself
	)";
	
	core::Dictionary<std::string> vm = parser.parse(test2, argc, argv, "alpha 1.0");
//
//	if (vm.has_key("name"))
//	{
//		std::string value;
//		vm.get("username", value);
//		fprintf(stdout, "name is: %s\n", value.c_str());
//	}
}


void test_main(int argc, char** argv)
{
//	test_rendering();
	//	test_hash();
	test_args(argc, argv);
}

int main(int argc, char** argv)
{
	platform::startup();
	core::startup();

	test_main(argc, argv);

	core::shutdown();
	platform::shutdown();
	return 0;
}
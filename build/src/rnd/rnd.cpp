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



struct Variant
{
	
};


struct ArgumentType
{
	typedef enum
	{
		Integer,
		Float,
		String
	} value;
};






class ArgumentParser
{
	struct Argument
	{
		// is this argument required?
		bool required;

		std::string name;
		
		Argument() : required(false)
		{
		}
	};



	struct ArgumentEntry
	{
		// is this a command argument?
		bool is_command;
		std::string command_name;
		
		std::vector<Argument> arguments;
		const char* line;
		
		ArgumentEntry() : is_command(false), line(0)
		{
		}
	};

	
	
	std::vector<ArgumentEntry> entries;




public:
	struct ArgumentOption
	{
		ArgumentParser* owner;
		
		ArgumentOption(ArgumentParser* parent) : owner(parent)
		{
		}
	
		ArgumentOption& operator()(const char* str, const char* help_string = 0)
		{
			owner->parse_string(str, help_string);
			return *this;
		}
	};
	
	
	void parse_string(const char* str, const char* help_string)
	{
//		fprintf(stdout, "%s\n", str);
		const char* cur = str;
		
		// states:
		/*
			- 0: scan
			- 1: command token
			- 2: required token
			- 3: option
		*/
		
		int state = 0;
		
		ArgumentEntry entry;
		entry.line = str;
		
		const char* begin = 0, *end = 0;
		
		const char* next = 0;
		while (*cur)
		{
			if (state == 0)
			{
				if (isalpha(*cur))
				{
					state = 1;
					begin = cur;
					// commands must be the FIRST option listed
					assert(entry.arguments.empty());
					entry.is_command = true;
				}
				else if (*cur == '<')
				{
					state = 2;
					begin = cur+1;
				}
			}
			else if (state == 1)
			{
				if (isspace(*cur))
				{
					state = 0;
					end = cur;
//					fprintf(stdout, "%.*s\n", (int)(end-begin), begin);
					
					entry.command_name = std::string(begin, (end-begin));
				}
			}
			else if (state == 2)
			{
				bool term_kwarg = *cur == '>';
				if (term_kwarg)
				{
					state = 0;
					end = cur;
//					fprintf(stdout, "%.*s\n", (int)(end-begin), begin);
					
					Argument ad;
					ad.required = true;
					ad.name = std::string(begin, (end-begin));
					entry.arguments.push_back(ad);
				}
			}
			
			cur++;
		}
		
		entries.push_back(entry);
	}

	ArgumentParser::ArgumentOption set_options()
	{
		return ArgumentOption(this);
	}
	
	void parse(int argc, char** argv, core::Dictionary<std::string>& dict)
	{
//		if (index > argc-1)
//			return;
		
		std::vector<std::string> tokens;
		for (int i = 1; i < argc; ++i)
		{
			tokens.push_back(argv[i]);
//			fprintf(stdout, "token: %s\n", argv[i]);
		}
		
		// scan through all entries
		bool failure = true;
		size_t index = 0;
		for (ArgumentEntry& entry : entries)
		{
			if (entry.is_command)
			{
				if (!tokens.empty() && (entry.command_name == tokens[index]))
				{
					fprintf(stdout, "found command: %s\n", entry.command_name.c_str());
					int diff = entry.arguments.size() - (tokens.size()-1);
					if (diff != 0)
					{
						fprintf(stdout, "argument count doesn't match!, diff: %i\n", diff);
						if (diff > 0)
						{
							for (int i = diff-1; i < diff; ++i)
							{
								fprintf(stdout, "missing argument: <%s>\n", entry.arguments[i].name.c_str());
							}
						}
						else
						{
							fprintf(stdout, "Unexpected argument(s)\n");
						}
						break;
					}
					else
					{
						dict.insert(entry.command_name.c_str(), "true");
						failure = false;
						int token_index = 1;
						for (Argument arg : entry.arguments)
						{
							dict.insert(arg.name.c_str(), tokens[token_index++]);
						}
						break;
					}
				}
			}
			else if (!tokens.empty())
			{
				fprintf(stdout, "trying to match entry: %i, total args: %i\n", index, entry.arguments.size());
				int diff = entry.arguments.size() - tokens.size();
				if (diff != 0)
				{
					fprintf(stdout, "argument count doesn't match!, diff: %i\n", diff);
					if (diff > 0)
					{
						for (int i = tokens.size(); i < entry.arguments.size(); ++i)
						{
							fprintf(stdout, "missing argument: <%s>\n", entry.arguments[i].name.c_str());
						}
					}
					else
					{
						fprintf(stdout, "Unexpected argument(s)\n");
					}
				}
				else
				{
					failure = false;
				}
			}
		
			++index;
		}
		
		if (failure)
		{
			// print all entry source strings
			fprintf(stdout, "USAGE:\n");
			for (ArgumentEntry& e : entries)
			{
				fprintf(stdout, "%s\n", e.line);
			}
		}
	}
};

void test_args(int argc, char** argv)
{
	ArgumentParser parser;

	parser.set_options()
	("name <username>")
	("<source_asset_root> <relative_asset_file> <destination_asset_root>")
	;


//	parser.set_options()
//	("tcp <host> <port>")
//	("serial <port> [--baud=9600] [--timeout=<seconds>]")
//	("-h | --help | --version")
//	;
	
	core::Dictionary<std::string> vm;
	parser.parse(argc, argv, vm);
	
	if (vm.has_key("name"))
	{
		std::string value;
		vm.get("username", value);
		fprintf(stdout, "name is: %s\n", value.c_str());
	}
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
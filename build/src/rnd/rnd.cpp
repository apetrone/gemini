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



#include <regex>

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


using namespace core::argparse;





class ArgumentParser
{
public:
	std::vector<Required*> usage_patterns;
	
	ArgumentParser() {}
	~ArgumentParser()
	{
		for (Required* p : usage_patterns)
		{
			delete p;
		}
		usage_patterns.clear();
	}

	struct ArgumentOption
	{
		ArgumentParser* owner;
		
		ArgumentOption(ArgumentParser* parent) : owner(parent)
		{
		}
	
		ArgumentOption& operator()(const char* str, const char* help_string = 0)
		{
			owner->parse_option(str, help_string);
			return *this;
		}
	};
	
	
	struct ArgumentUsage
	{
		ArgumentParser* owner;
		
		ArgumentUsage(ArgumentParser* parser) : owner(parser)
		{
		}
		
		ArgumentUsage& operator()(const char* str, const char* help_string = 0)
		{
			owner->parse_usage(str, help_string);
			return *this;
		}
	};
	
	
	
	void try_parse(TokenWrapper& tokens, PatternList& patterns)
	{
		int argument_index = 0;
		int pattern_index = 0;
		

		// first, we build a list of the input tokens and categorize these
		// into a series of patterns.
		size_t total_tokens = tokens.size();
		while (true)
		{
			const std::string& arg = tokens.current();
			
			if (arg == "")
			{
				LOGV("end of arguments\n");
				break;
			}
			
			if (arg == "--")
			{
				// the following arguments are positional
				LOGV("The following arguments will be treated as positional arguments\n");
				tokens.pop();
				break;
			}
			else if (starts_with("--", arg))
			{
				// parse a long option
				LOGV("\"%s\" is a long option\n", arg.c_str());
				Option* option = parse_long(tokens);
				patterns.push_back(option);
			}
			else if (starts_with("-", arg))
			{
				// parse a short option
				LOGV("\"%s\" is a short option (or group of)\n", arg.c_str());
				tokens.pop();
			}
			else
			{
				// parse an argument
				LOGV("\"%s\" is an argument\n", arg.c_str());
				Argument* newargument = new Argument("", arg);
				tokens.pop();
				patterns.push_back(newargument);
			}
		}
		
		// now that info.input is populated, we can determine how to map
		// them to the usage branches.
		
		
	}

	Option* parse_long(TokenWrapper& tokens)
	{
		int total_arguments = 0;

		// long ::= '--' chars [ ( ' ' | '=' ) chars ] ;
		// split at the '=', if one exists.
		std::string long_arg = tokens.pop();

		// partition: '--longname=<argument>'
		// to: ('--longname', '=', '<argument>')


		// TODO: check options for matches by name
		// TODO: check for non-unique prefix


		Option* option = 0;
		
		int found_options = 0;
		
		std::string longname = long_arg;
		std::string value;
	
		if (found_options > 1)
		{
			// TODO: warn about non unique option prefix
			// specified ambiguously 2+ times?
		}
		else if (found_options == 0)
		{
			size_t eq_pos = long_arg.find('=');
			if (eq_pos != std::string::npos)
			{
				longname = long_arg.substr(0, eq_pos);
				total_arguments = 1;
				value = long_arg.substr(eq_pos+1);
			}
			
			option = new Option("", longname, total_arguments, value);
			// TODO: add to main options list
		}
		else
		{
			// TODO: grab the existing option
			// if it accepts an argument, then consume one (if we can)
		}

		return option;
	}

	void parse_atom(TokenWrapper& tokens, PatternList& results)
	{
		// atom ::= '(' expr ')' | '[' expr ']' | 'options'
		//	| longname | shortnames | argument | command ;
		const std::string& token = tokens.current();

		if (token == "(" || token == "[")
		{
			tokens.pop();
			if (token == "(")
			{
				LOGV("TODO: implement parentheses!\n");
			}
			else if (token == "[")
			{
				PatternList option_results;
				parse_expr(tokens, option_results);
				if (tokens.current() != "]")
				{
					LOGV("Unmatched '%s'\n", "]");
				}
				tokens.pop();
				Optional* optional = new Optional(option_results);
				results.push_back(optional);
				return;
			}
		}
		else if (starts_with("--", token) && token != "--")
		{
			//LOGV("this is a long option\n");
			Option* option = parse_long(tokens);
			results.push_back(option);
		}
		else if (starts_with("-", token) && token != "-")
		{
			//LOGV("this is a short option\n");
			tokens.pop();
		}
		else if (starts_with("<", token) && ends_with(">", token))
		{
			//LOGV("this is an argument\n");
			::Argument* argument = new ::Argument(tokens.pop(), "");
			results.push_back(argument);
		}
		else
		{
			//LOGV("this is a command\n");
			Command* command = new Command(tokens.pop(), "");
			results.push_back(command);
		}
	}

	void parse_sequence(TokenWrapper& tokens, PatternList& results)
	{
		// sequence ::= ( atom [ '...' ] )* ;
		while (true)
		{
			std::string current = tokens.current();
			if (current == "" || current == "]" || current == ")" || current == "|")
			{
				//LOGV("detected end of sequence\n");
				break;
			}

			parse_atom(tokens, results);
		}
	}

	void parse_expr(TokenWrapper& tokens, PatternList& results)
	{
		// expr ::= sequence ( '|' sequence )* ;

		parse_sequence(tokens, results);

		//while (tokens.current() == "|")
		//{
		//	tokens.pop();
		//	PatternList other_results;
		//	parse_sequence(tokens, other_results);
		//	results.push_back(new Required(other_results));
		//}

		
	}

	void parse_option(const std::string& option, const char* help_string)
	{
		LOGV("option: %s\n", option.c_str());
	}

	void parse_usage(const std::string& formal_usage, const char* help_string)
	{
		// the formal usage pattern needs to be split up into tokens
		// which can be fed into a regex.
		LOGV("formal_usage: %s\n", formal_usage.c_str());

		//std::regex test("");
		//std::cmatch result;
		//std::regex_match()

		std::string str;

		// first, we expand the usage string so we have extra whitespace
		// to separate elements.
		std::regex replacement("([\\[\\]\\(\\)\\|]|\\.\\.\\.)");
		std::regex_replace(std::back_inserter(str), formal_usage.begin(), formal_usage.end(), replacement, " $1 ");

		LOGV("str: %s\n", str.c_str());

		TokenList usage;

		std::regex rgx("\\s+");
		std::sregex_token_iterator iter(str.begin(),
			str.end(),
			rgx,
			-1);
		std::sregex_token_iterator end;

		// next, we tokenize the expanded string
		for (; iter != end; ++iter)
		{
			//LOGV("t: %s\n", (*iter).str().c_str());
			usage.push_back( (*iter).str() );
		}

		// now we parse the expanded string
		TokenWrapper tw(usage, std::string(""));
		PatternList usage_pattern;
		parse_expr(tw, usage_pattern);

		//for (Pattern* pattern : usage_pattern)
		//{
		//	LOGV("pattern type: %s\n", pattern->get_classname());
		//}

		usage_patterns.push_back(new Required(usage_pattern));
	}
	
	ArgumentParser::ArgumentUsage set_usage()
	{
		return ArgumentUsage(this);
	}

	ArgumentParser::ArgumentOption set_options()
	{
		return ArgumentOption(this);
	}
	
	void parse(int argc, char** argv, core::Dictionary<std::string>& dict)
	{
		TokenList tokens;
		for (int i = 1; i < argc; ++i)
		{
			tokens.push_back(argv[i]);
		}
		

		TokenWrapper tokenwrapper(tokens, std::string(""));
		PatternList patterns;
		try_parse(tokenwrapper, patterns);
		
		
		
		
		// scan through all entries
		bool failure = true;
		
		//if (failure)
		//{
		//	// print all entry source strings
		//	fprintf(stdout, "USAGE:\n");
		//	for (ArgumentEntry& e : entries)
		//	{
		//		fprintf(stdout, "%s\n", e.line);
		//	}
		//}
		
		
		PatternList input_items;
#if 0
		Argument a("", "tcp"); input_items.push_back(&a);
		Argument b("", "localhost"); input_items.push_back(&b);
		Argument c("", "27015"); input_items.push_back(&c);
		Option d("", "--timeout", 1, "30"); input_items.push_back(&d);
		Option baud("", "--baud", 1, "4500"); input_items.push_back(&baud);
		//		Option f("-f", "--file", 1, "/dev/USBtty0"); input_items.push_back(&f);
#elif 0
		Argument a("", "ship"); input_items.push_back(&a);
		Argument b("", "adam"); input_items.push_back(&b);
		Option speed("", "--speed", 1, "100"); input_items.push_back(&speed);
		Argument c("", "move"); input_items.push_back(&c);
		Argument d("", "30"); input_items.push_back(&d);
		Argument e("", "50"); input_items.push_back(&e);
		//Argument f("", "100"); input_items.push_back(&f);
		
#endif

#if 0
		PatternWrapper input(input_items, nullptr);

		for (Required* usage : usage_patterns)
		{
			bool matches = usage->matches(input);
			LOGV("matches: %i\n", matches);

			// if matches == 0; we didn't match all required arguments
			// dump usage strings

			// too many arguments specified
			if (input.size() > 0)
			{
				LOGV("ignoring unrecognized arguments\n");
			}
		}
#endif
	}

};



void test_args(int argc, char** argv)
{
	ArgumentParser parser;
	
	// Options can be specified in any order.
	
	parser.set_usage()
	//("tcp <host> <port>")
	("tcp <host> <port> [--baud=9600] [--timeout=<seconds>]")
	//("serial <port> [--baud=9600] [--timeout=<seconds>|--block] [<name>]")
	//("-h | --help | --version")
	;


#if 0
	parser.set_options()
	("-f FILE", "file name")
	;
#endif

//	parser.set_options()
	//("ship new <name>...")
	//("ship <name> [--speed=<knots>] move <x> <y>")
//	;

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
	//parser.set_options()
	//("export [--animation] <assets_root> (-d|--destination)=<assets_destination>")
	//;

//	parser.set_options()
//	("[-t]")
//	("[-a|--animation]")
//	("[-h|--help|--version]")
//	("[-h | --help | --verbose]")
//	("[-v | --verbose-only]")
//	("[-f | --frame=<seconds>]")
//	("tcp <host> <port> [--timeout=<seconds>]")
//	("serial <port> [--baud=9600] [--timeout=<seconds>]")
//	("-h | --help | --version")

//	("name [-a|--animation-only] <username>")
//	("export <source_asset_root> <relative_asset_file> <destination_asset_root>")
//	;


//	parser.set_options()
//	("tcp <host> <port>")
//	("serial <port> [--baud=9600] [--timeout=<seconds>]")
//	("-h | --help | --version")
//	;
	
	core::Dictionary<std::string> vm;
	parser.parse(argc, argv, vm);
//
//	if (vm.has_key("name"))
//	{
//		std::string value;
//		vm.get("username", value);
//		fprintf(stdout, "name is: %s\n", value.c_str());
//	}

	/*
	parser.set_options()
	("-f FILE"  Specify the file name")
	("--no-rtti  Disable RTTI")
	("--with-vr  Enable Virtual Reality HMD")
	("--animation-only  Export animations only")
	("--skeleton-only  Export skeletons only")
	;
	*/
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
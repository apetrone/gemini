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
	std::vector<Option*> options_registry;
	
	ArgumentParser() {}
	~ArgumentParser()
	{
		for (Required* p : usage_patterns)
		{
			delete p;
		}
		usage_patterns.clear();
		
		for (Option* o : options_registry)
		{
			delete o;
		}
		options_registry.clear();
	}
	
	
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
				LOGV("The following arguments will be treated as positional\n");
				tokens.pop();
				break;
			}
			else if (starts_with("--", arg))
			{
				// parse a long option
				LOGV("\"%s\" is a long option\n", arg.c_str());
				Option* option = parse_long(tokens, options_registry);
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
	
	Option* find_option(std::vector<Option*>& options,
						const std::string& shortname,
						const std::string& longname,
						int& found_options)
	{
		Option* option = 0;
		
		bool test_shortname = !shortname.empty();
		bool test_longname = !longname.empty();
		
		for (Option* o : options)
		{
			if (test_shortname && o->get_name() == shortname)
			{
				option = o;
				++found_options;
			}
			else if (test_longname && o->longname == longname)
			{
				option = o;
				++found_options;
			}
		}
		return option;
	}

	Option* parse_long(TokenWrapper& tokens, std::vector<Option*>& options)
	{
		// long ::= '--' chars [ ( ' ' | '=' ) chars ] ;
		// split at the '=', if one exists.
		std::string long_arg = tokens.pop();

		// partition: '--longname=<argument>'
		// to: ('--longname', '=', '<argument>')


		Option* option = 0;
		int found_options = 0;
		std::string longname;
		std::string value;
		int total_arguments = 0;
		option = find_option(options, "", longname, found_options);
		

		
		size_t eq_pos = long_arg.find('=');
		if (eq_pos != std::string::npos)
		{
			longname = long_arg.substr(0, eq_pos);
			total_arguments = 1;
			value = long_arg.substr(eq_pos+1);
		}
		else
		{
			longname = long_arg;
		}
		
		
	
		if (found_options > 1)
		{
			// TODO: warn about non unique option prefix
			// specified ambiguously 2+ times?
		}
		else if (found_options == 0)
		{
			option = new Option("", longname, total_arguments, value);
			options.push_back(option);
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
			Option* option = parse_long(tokens, options_registry);
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

	
	std::string get_section_regex(const std::string& name)
	{
		std::string regex = "(^\\s)*(";
		
		regex += name;
		
		regex += "[^\\n]*\\n?(?:[ \\t].*?(?:\\n|$))*)";
		
		return regex;
	}
	
	std::vector<std::string> split(const std::string& input, const std::string& delimiter)
	{
		std::vector<std::string> elements;
		
		std::string::size_type pos = 0, last = 0;
		
		using value_type = std::vector<std::string>::value_type;
		using size_type = std::vector<std::string>::size_type;

		bool end_of_string = false;
		while(true)
		{
			pos = input.find_first_of(delimiter, last);
			if (pos == std::string::npos)
			{
				pos = input.length();
				end_of_string = true;
				break;
			}
			
			if (pos != last)
			{
				elements.push_back(value_type(input.data()+last, (size_type)pos-last));
			}
			
			if (end_of_string)
			{
				break;
			}
			
			last = pos+1;
		}
		
		return elements;
	}
	
	std::string trim_left(const std::string& input, const std::string& chars = "\t ")
	{
		std::string out;
		
		std::size_t start = input.find_first_not_of(" \t");
		if (start != std::string::npos)
		{
			out = input.substr(start);
		}
		else
		{
			out = input;
		}
		
		return out;
	}
	
	std::vector<std::string> parse_section(const char* docstring, const std::string& section_name)
	{
		std::regex rgx(get_section_regex(section_name));
		std::cmatch result;
		std::regex_search(docstring, result, rgx);
		
		std::vector<std::string> output;

		if (result.size() > 0)
		{
			std::vector<std::string> lines;
			std::string match = result.str(0);
			std::size_t pos = match.find_first_of(':');
			if (pos != std::string::npos)
			{
				match = match.substr(pos+1);
			}
			lines = split(match, "\n");
			
			for (std::string& line : lines)
			{
				output.push_back(trim_left(line));
			}
		}
		
		return output;
	}
	
	void parse_options(std::vector<std::string> lines)
	{
		for (auto& s : lines)
		{
			LOGV("%s\n", s.c_str());
		}
	}
	
	void parse(const char* docstring, int argc, char** argv, core::Dictionary<std::string>& dict)
	{
		// run through the docstring and parse the options first.
		parse_options(parse_section(docstring, "Options"));

	
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

	ArgumentParser parser;
	
	const char* test1 = R"(
Usage:
	tcp <host> <port> [--baud=9600] [--timeout=<seconds>]
	serial <port> [--baud=9600] [--timeout=<seconds>]
	
Options:
	-h, --help	Display this help string
	--version 	Show the program version
	)";
	

	core::Dictionary<std::string> vm;
	parser.parse(test1, argc, argv, vm);
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
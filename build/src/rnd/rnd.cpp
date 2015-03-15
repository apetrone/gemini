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

// Proof of concept inspired by docopt library for Python by Vladimir Keleshev
// Must be C++
// Must not use exceptions
// Must compile with gcc4.8, clang, vs2010/vs2013

struct TokenInfo;
struct Pattern;

typedef std::vector<std::string> TokenList;
typedef std::vector<Pattern*> PatternList;


template <class T>
struct Wrapper
{
	typedef std::vector<T> ContainerType;
	ContainerType items;
	size_t index;
	const T default_value;
	size_t items_length;

	Wrapper(ContainerType& input_array, const T& default_value) :
		items(input_array),
		index(0),
		default_value(default_value)
	{

		items_length = items.size();
	}

	T current() const
	{
		if (items.empty())
		{
			return default_value;
		}

		if (index < items.size())
		{
			return items[index];
		}

		return default_value;
	}

	T pop()
	{
		if (items.empty())
		{
			return default_value;
		}

		return items[index++];
	}

	T& at(size_t index) const
	{
		return const_cast<T&>(items[index]);
	}

	size_t size() const
	{
		return items_length - index;
	}

	class Iterator
	{
	private:
		const Wrapper<T>& container;
		unsigned int index;

	public:
		Iterator(const Wrapper<T>& container, size_t index = 0) :
			container(container),
			index(index)
		{
		}

		bool operator!=(const Iterator& other)
		{
			return (index != other.index);
		}

		const Iterator& operator++()
		{
			++index;
			return *this;
		}

		T& operator*() const
		{
			return container.at(index);
		}

		Iterator& operator=(const Iterator& other)
		{
			//this->container = other.container;
			this->index = other.index;
			return *this;
		}

		size_t get_index() const { return index; }
	};

	Iterator erase(Iterator& it)
	{
		items.erase(items.begin()+it.get_index());
		--items_length;
		return Iterator(*this, index);
	}

	Iterator begin() const { return Iterator(*this, index); }
	Iterator end() const { return Iterator(*this, items_length); }
};



typedef Wrapper<std::string> TokenWrapper;
typedef Wrapper<Pattern*> PatternWrapper;

struct Pattern
{
	enum Type
	{
		PT_Pattern,
		PT_Argument,
		PT_Command,
		PT_Option,

		PT_LeafPattern,
		PT_BranchPattern
	};


	virtual bool is_leaf() const { return false; }
	virtual bool is_branch() const { return false; }
	virtual bool matches(PatternWrapper& patterns) { return 0; }
	virtual const char* get_classname() const { return "Pattern"; }
	virtual Type get_type() const { return PT_Pattern; }

	template <class T>
	T* cast() { return static_cast<T*>(this); }
};


struct TokenInfo
{
	int argument_index;
	int pattern_index;
	TokenList& arguments;
	PatternList input;

	TokenInfo(TokenList& input_arguments) :
		arguments(input_arguments),
		argument_index(0),
		pattern_index(0)
	{
	}

	~TokenInfo()
	{
		for (Pattern* p : input)
		{
			delete p;
		}
		input.clear();
	}
};



bool starts_with(const std::string& prefix, const std::string& str)
{
	std::string s2 = str.substr(0, prefix.length());
	return (prefix == s2);
}

bool ends_with(const std::string& postfix, const std::string& str)
{
	std::string s2 = str.substr(str.length() - postfix.length(), postfix.length());
	return (postfix == s2);
}



struct LeafPattern : public Pattern
{
protected:
	std::string name;
	std::string value;
	
public:
	virtual bool is_leaf() const { return true; }
	
	virtual const std::string& get_name() const { return name; }
	virtual const std::string& get_value() const { return value; }

	virtual const char* get_classname() const { return "LeafPattern"; }
	virtual Type get_type() const { return PT_LeafPattern; }

	virtual bool matches(PatternWrapper& patterns);
	virtual bool single_match(PatternWrapper& patterns);
};


struct Argument : public LeafPattern
{
	Argument(const std::string& input_name, const std::string& input_value)
	{
		name = input_name;
		value = input_value;
	}

	virtual bool single_match(PatternWrapper& patterns)
	{
		for (Pattern* p : patterns)
		{
			if (p->get_type() == PT_Argument)
			{
				patterns.pop();
				return true;
			}
		}
		return false;
	}

	virtual const char* get_classname() const { return "Argument"; }
	virtual Type get_type() const { return PT_Argument; }
};




struct Option : public LeafPattern
{
	std::string longname;
	int total_arguments;

	Option(const std::string& shortname, 
		const std::string& longname = "", 
		int argument_count = 0,
		const std::string& value = "")
	{
		this->name = shortname;
		this->longname = longname;
		this->value = value;
		this->total_arguments = argument_count;
	}

	virtual const char* get_classname() const { return "Option"; }
	virtual Type get_type() const { return PT_Option; }

	virtual bool single_match(PatternWrapper& patterns)
	{
		bool matched = false;
		for (PatternWrapper::Iterator it = std::begin(patterns); it != std::end(patterns); ++it)
		{
			Pattern* p = (*it);
			if (p->get_type() == PT_Option)
			{
				Option* option = p->cast<Option>();
				if (this->longname == option->longname)
				{
					LOGV("matched: %s -> '%s'\n", option->longname.c_str(), option->get_value().c_str());
					//patterns.pop();
					// we cannot pop, but we can erase.
					it = patterns.erase(it);
					return true;
				}
			}
			
			//LOGV("class: %s\n", p->get_classname());
		}
		return matched;
	}
};

struct Command : public Argument
{
	Command(const std::string& input_name, const std::string& input_value) : Argument(input_name, input_value)
	{
	}

	virtual const char* get_classname() const { return "Command"; }
	virtual Type get_type() const { return PT_Command; }

	virtual bool single_match(PatternWrapper& patterns)
	{
		for (Pattern* p : patterns)
		{
			if (p->get_type() == PT_Argument)
			{
				Argument* argument = p->cast<Argument>();
				if (argument->get_value() == this->name)
				{
					patterns.pop();
					return true;
				}				
			}
		}
		return false;
	}
};


struct BranchPattern : public Pattern
{
	PatternList children;

	BranchPattern(const PatternList& child_list) : children(child_list)
	{
	}

	virtual bool is_branch() const { return true; }
	virtual const char* get_classname() const { return "BranchPattern"; }
	virtual Type get_type() const { return PT_BranchPattern; }
};

struct Required : public BranchPattern
{
	Required(const PatternList& child_list) : BranchPattern(child_list)
	{
	}

	virtual bool matches(PatternWrapper& patterns)
	{
		bool matched = false;
		for(Pattern* child : children)
		{
			matched = child->matches(patterns);
			if (!matched)
			{
				return false;
			}
		}
		
		return matched;
	}

	virtual const char* get_classname() const { return "Required"; }
};

struct Optional : public BranchPattern
{
	Optional(const PatternList& child_list) : BranchPattern(child_list)
	{
	}


	virtual bool matches(PatternWrapper& patterns)
	{
		for (Pattern* child : children)
		{
			child->matches(patterns);
		}
		return true;
	}

	virtual const char* get_classname() const { return "Optional"; }
};

struct OneOrMore : public BranchPattern
{
	virtual const char* get_classname() const { return "OneOrMore"; }
};

struct Either : public BranchPattern
{
	Pattern* left;
	Pattern* right;

	virtual const char* get_classname() const { return "Either"; }
};


bool LeafPattern::matches(PatternWrapper& patterns)
{
	bool matched = false;
	if (this->get_type() == PT_Argument || this->get_type() == PT_Command || this->get_type() == PT_Option)
	{
		LeafPattern* leaf = this->cast<LeafPattern>();
		matched = leaf->single_match(patterns);
	}

	return matched;
}

bool LeafPattern::single_match(PatternWrapper& patterns)
{
	// should not get here.
	assert(0);
	return false;
}


class ArgumentParser
{
public:
	Pattern* root;

	struct ArgumentOption
	{
		ArgumentParser* owner;
		
		ArgumentOption(ArgumentParser* parent) : owner(parent)
		{
		}
	
		ArgumentOption& operator()(const char* str, const char* help_string = 0)
		{
			owner->parse_usage(str, help_string);
			return *this;
		}
	};
	
	
	//struct ArgumentUsage
	//{
	//	ArgumentParser* owner;
	//	
	//	ArgumentUsage(ArgumentParser* parser) : owner(parser)
	//	{
	//	}
	//};

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

		size_t eq_pos = long_arg.find('=');
		if (eq_pos != std::string::npos)
		{
			std::string longname = long_arg.substr(0, eq_pos);

			total_arguments = 1;
			Option* option = new Option("", longname, total_arguments);
			// TODO: add to main options list

			return option;
		}

		// We haven't reached this far yet!
		assert(0);

		return 0;
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

		PatternList input_items;
#if 1
		Argument a("", "tcp"); input_items.push_back(&a);
		Argument b("", "localhost"); input_items.push_back(&b);
		Argument c("", "27015"); input_items.push_back(&c);
		Option d("", "--timeout", 1, "30"); input_items.push_back(&d);
		Option baud("", "--baud", 1, "4500"); input_items.push_back(&baud);
		Option f("-f", "--file", 1, "/dev/USBtty0"); input_items.push_back(&f);
#elif 1
		Argument a("", "ship"); input_items.push_back(&a);
		Argument b("", "adam"); input_items.push_back(&b);
		Option speed("", "--speed", 1, "100"); input_items.push_back(&speed);
		Argument c("", "move"); input_items.push_back(&c);
		Argument d("", "30"); input_items.push_back(&d);
		Argument e("", "50"); input_items.push_back(&e);
		//Argument f("", "100"); input_items.push_back(&f);
		
#endif

		PatternWrapper input(input_items, nullptr);

		Required pattern(usage_pattern);

		bool matches = pattern.matches(input);
		LOGV("matches: %i\n", matches);

		// if matches == 0; we didn't match all required arguments

		// too many arguments specified
		if (input.size() > 0)
		{
			LOGV("ignoring unrecognized arguments\n");
		}
	}
	
	//ArgumentParser::ArgumentUsage set_usage()
	//{
	//	return ArgumentUsage(this);
	//}

	ArgumentParser::ArgumentOption set_options()
	{
		return ArgumentOption(this);
	}
	
	void parse(int argc, char** argv, core::Dictionary<std::string>& dict)
	{
		std::vector<std::string> tokens;
		for (int i = 1; i < argc; ++i)
		{
			tokens.push_back(argv[i]);
			fprintf(stdout, "token: %s\n", argv[i]);
		}
		
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
	}
};

void try_parse(PatternList& patterns, TokenInfo& info)
{
	// first, we build a list of the input tokens and categorize these
	// into a series of patterns.
	size_t total_tokens = info.arguments.size();
	while (true)
	{
		if (info.argument_index > (total_tokens - 1))
		{
			LOGV("end of arguments\n");
			break;
		}

		const std::string& arg = info.arguments[info.argument_index];
		Pattern* pattern = patterns[info.pattern_index];

		if (arg == "--")
		{
			// the following arguments are positional
			LOGV("The following arguments will be treated as positional arguments\n");
			break;
		}
		else if (starts_with("--", arg))
		{
			// parse a long option
			LOGV("\"%s\" is a long option\n", arg.c_str());
			++info.argument_index;
		}
		else if (starts_with("-", arg))
		{
			// parse a short option
			LOGV("\"%s\" is a short option (or group of)\n", arg.c_str());
			++info.argument_index;
		}
		else
		{
			// parse an argument
			LOGV("\"%s\" is an argument\n", arg.c_str());
			++info.argument_index;
			Argument* newargument = new Argument("", arg);
			info.input.push_back(newargument);
		}
	}

	// now that info.input is populated, we can determine how to map
	// them to the usage branches.


}

void test_args(int argc, char** argv)
{
	ArgumentParser parser;
	
	// Options can be specified in any order.
	
	parser.set_options()
	//("tcp <host> <port>")
	("tcp <host> <port> [--baud=9600] [--timeout=<seconds>]")
	//("serial <port> [--baud=9600] [--timeout=<seconds>|--block] [<name>]")
	//("-h | --help | --version")
	;

	parser.set_options()
	//("ship new <name>...")
	//("ship <name> [--speed=<knots>] move <x> <y>")
	;

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
	;


//	parser.set_options()
//	("tcp <host> <port>")
//	("serial <port> [--baud=9600] [--timeout=<seconds>]")
//	("-h | --help | --version")
//	;
	
//	core::Dictionary<std::string> vm;
//	parser.parse(argc, argv, vm);
//	
//	if (vm.has_key("name"))
//	{
//		std::string value;
//		vm.get("username", value);
//		fprintf(stdout, "name is: %s\n", value.c_str());
//	}





	//PatternList patterns;
	//
	//Command serial("serial", "");
	//patterns.push_back(&serial);

	//Argument port("port", "");
	//patterns.push_back(&port);

	////Optional baud;
	////patterns.push_back(&baud);
	//
	////Optional timeout;
	////patterns.push_back(&timeout);

	//Required r0(patterns);
	//TokenList arguments;
	//arguments.push_back("serial");
	//arguments.push_back("10423");
	////arguments.push_back("--baud=9600");


	//TokenInfo tokeninfo(arguments);
	//try_parse(patterns, tokeninfo);
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
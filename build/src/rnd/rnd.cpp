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







struct Sequence
{
	float duration_seconds;
	
	typedef std::vector<KeyframeList> Track;

	std::vector<Track> tracks;
};

typedef uint32_t SequenceId;


Sequence* get_sequence(SequenceId id)
{
	static Sequence* _sequence = new Sequence();
	return _sequence;
}


struct AnimatedPose
{
	glm::vec3 scale;
	glm::vec3 rotation;
	glm::vec3 translation;

	Channel sx, sy, sz;
	Channel rx, ry, rz;
	Channel tx, ty, tz;
	
	AnimatedPose(bool wrap = true) :
		sx(scale.x, wrap), sy(scale.y, wrap), sz(scale.z, wrap),
		rx(rotation.x, wrap), ry(rotation.y, wrap), rz(rotation.z, wrap),
		tx(translation.x, wrap), ty(translation.y, wrap), tz(translation.z, wrap)
	{
	}
};

struct SequenceInstance
{
	SequenceId sequence;

	
	AnimatedPose* poses;
	
	// normalized local time [0,1]
	float normalized_time;
	

	float playback_rate;
	
	SequenceInstance() :
		poses(0)
	{
		normalized_time = 0.0f;
		playback_rate = 1.0f;
	}
	
	~SequenceInstance()
	{
		Sequence* seq = get_sequence(sequence);
		DESTROY_ARRAY(AnimatedPose, poses, seq->tracks.size());
	}
	
	void init_with_sequence(SequenceId id)
	{
		// assume whatever.
		sequence = id;
		Sequence* seq = get_sequence(id);
		
		size_t total_tracks = seq->tracks.size();
		poses = CREATE_ARRAY(AnimatedPose, total_tracks);

		for (size_t index = 0; index < total_tracks; ++index)
		{
			// set the keyframe list for each channel
			poses[index].sx.set_keyframe_list(&seq->tracks[0][index]);
			poses[index].sy.set_keyframe_list(&seq->tracks[1][index]);
			poses[index].sz.set_keyframe_list(&seq->tracks[2][index]);
			
			poses[index].rx.set_keyframe_list(&seq->tracks[3][index]);
			poses[index].ry.set_keyframe_list(&seq->tracks[4][index]);
			poses[index].rz.set_keyframe_list(&seq->tracks[5][index]);
			
			poses[index].tx.set_keyframe_list(&seq->tracks[6][index]);
			poses[index].ty.set_keyframe_list(&seq->tracks[7][index]);
			poses[index].tz.set_keyframe_list(&seq->tracks[8][index]);
		}
	}
	
	void advance(float delta_seconds)
	{
		float dt = delta_seconds * playback_rate;
		
		// advance all channels here
		for (size_t index = 0; index < 1; ++index)
		{
			poses[index].sx.advance(dt);
			poses[index].sy.advance(dt);
			poses[index].sz.advance(dt);
			
			poses[index].rx.advance(dt);
			poses[index].ry.advance(dt);
			poses[index].rz.advance(dt);
			
			poses[index].tx.advance(dt);
			poses[index].ty.advance(dt);
			poses[index].tz.advance(dt);
		}
	}
};


void test_main()
{
	//	test_rendering();
	//	test_hash();
	
	Keyframe keys[2];
	keys[0].seconds = 0.0f;
	keys[0].value = 0.0f;
	keys[1].seconds = 1.0f;
	keys[1].value = 10.0f;
	
	
	KeyframeList kfl;
	kfl.allocate(2);
	kfl.duration_seconds = 1.0f;
	kfl.set_key(0, keys[0].seconds, keys[0].value);
	kfl.set_key(1, keys[1].seconds, keys[1].value);
	
	float xposition = 0.0f;
	Channel px(xposition);
	px.set_keyframe_list(&kfl);

	for (uint32_t a = 0; a < 15; ++a)
	{
		px.advance(0.1f);
		
		LOGV("value: %2.2f\n", px());
	}
	
	//	Sequence* idle_sequence = animation::load_sequence("animations/idle");
	
	// Entity:
	//	SequenceInstance* idle = animation::create_instance("animations/idle");
	//	idle->play(1, &idle_completed);
	
	
	Sequence* sequence0 = get_sequence(0);
	sequence0->duration_seconds = 1.0f;
	//	sequence0->tx.allocate(1);
	//	sequence0->tx[0].allocate(2);
	//	KeyframeList* kfl = &sequence0->tx[0];
	//	kfl->set_key(0, 0.0f, 0.0f);
	//	kfl->set_key(1, 1.0f, 100.0f);
	
	SequenceInstance* s0 = new SequenceInstance();
	s0->init_with_sequence(0);
	
	delete s0;
	delete get_sequence(0);
}

int main(int argc, char** argv)
{
	platform::startup();
	core::startup();


	test_main();
	
	
	
	core::shutdown();
	platform::shutdown();
	return 0;
}
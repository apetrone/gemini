#include <stdio.h>
#include <stdlib.h>

#include <functional>
#include <vector>
#include <algorithm>

using namespace std;

#include <gemini/typedefs.h>
#include <gemini/mem.h>
#include <gemini/core.h>
#include <gemini/core/filesystem.h>
#include <gemini/core/log.h>
#include <gemini/util/stackstring.h>
#include <gemini/core/xfile.h>
#include <gemini/util/fixedarray.h>
#include <gemini/util/arg.h>

#include <slim/xlog.h>

#include <json/json.h>

#define USE_SDL2 1

#if USE_SDL2
	#include <SDL.h>
#endif


#if PLATFORM_MACOSX
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl.h>
#else
	#error Not implemented on this platform.
#endif

#include "common.h"

#include "keyframechannel.h"

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



struct Texture
{
	GLuint id;
	uint32_t width;
	uint32_t height;
	
	Texture()
	{
		glGenTextures(1, &id);
	}
	
	void activate()
	{
		glBindTexture(GL_TEXTURE_2D, id);
	}
	
	void setup(uint32_t _width, uint32_t _height, void* pixels = 0)
	{
		width = _width;
		
		height = _height;
		activate();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//	gl.GenerateMipmap(GL_TEXTURE_2D);
		float border[] = {1, 1, 1, 1};
		glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border );
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
				
		deactivate();
	}
	
	void deactivate()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	~Texture()
	{
		glDeleteTextures(1, &id);
	}
};

struct Framebuffer
{
	GLuint fbo;
	GLuint rbo;
	Texture* color_attachment;
	
	Framebuffer(Texture* color)
	{
		color_attachment = color;
		glGenFramebuffers(1, &fbo);
		
		activate();
		
		// bind the texture
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D,
			color_attachment->id,
			0
		);
		
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		LOGV("fbo status ok? %i\n", status==GL_FRAMEBUFFER_COMPLETE);
		
		deactivate();
	}
	
	~Framebuffer()
	{
		glDeleteFramebuffers(1, &fbo);
	}
	
	void activate()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}
	
	void deactivate()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};

void render_frame(Framebuffer& fbo, Texture& texture)
{
	fbo.activate();
	glViewport(0, 0, texture.width, texture.height);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glClearColor(1.0f, 0.25f, 0.35f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	fbo.deactivate();



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
	
	glEnable(GL_TEXTURE_2D);
	texture.activate();
	// draw a quad with a texture
	glBegin(GL_QUADS);
		glTexCoord2d(0, 0); glVertex2d(50, 50);
		glTexCoord2d(1, 0); glVertex2d(100, 50);
		glTexCoord2d(1, 1); glVertex2d(100, 100);
		glTexCoord2d(0, 1); glVertex2d(50, 100);
	glEnd();
	texture.deactivate();
	
	glDisable(GL_TEXTURE_2D);
}

void test_rendering()
{
	Texture* t0;
	Framebuffer* f0;
	

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

	
	t0 = new Texture();
	t0->setup(256, 256);
	
	f0 = new Framebuffer(t0);
	
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
		
		render_frame(*f0, *t0);
		
		SDL_GL_SwapWindow(window);
	}
	
	
	delete t0;
	delete f0;
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
#endif
}



int main(int argc, char** argv)
{
	tools::startup();
//	test_function();
//	test_rendering();
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
	
	tools::shutdown();
	return 0;
}
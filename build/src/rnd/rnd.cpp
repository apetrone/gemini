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
	#include <OpenGL/gl3.h>
#else
	#error Not implemented on this platform.
#endif

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

void render_frame()
{
	glViewport(0, 0, 800, 600);
	glClearColor(0.25f, 0.25f, 0.35f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void test_rendering()
{
#if USE_SDL2
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Window* window = 0;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	window = SDL_CreateWindow(
		"rnd",
		0, 0, 800, 600,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);
	
	SDL_GLContext context = SDL_GL_CreateContext(window);

				
	SDL_Event event;
	bool run = true;
	
	
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
		
		render_frame();
		
		SDL_GL_SwapWindow(window);
	}
	
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
#endif
}


int main(int argc, char** argv)
{
	memory::startup();
	core::startup();
//	test_function();
	test_rendering();
	
	core::shutdown();
	memory::shutdown();
	return 0;
}
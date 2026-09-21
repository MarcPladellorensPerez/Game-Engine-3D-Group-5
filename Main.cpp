#include <glad/glad.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>


int main(int argc, char* argv[]) {
	if (!SDL_Init(SDL_INIT_VIDEO))
		return -1;

	//Opengl attributes required to know before we create the window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); // desired version
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // we want a double buffer (it is the default)
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); // we want to have a depth buffer with minimum 24 bits
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8); // we want to have a stencil buffer with minimum 8 bits

	Uint32 flags = SDL_WINDOW_OPENGL;

	//window start params
	//#define WINDOW_FULLSCREEN
#define WINDOW_RESIZEABLE

#ifdef WINDOW_FULLSCREEN
	flags |= SDL_WINDOW_FULLSCREEN;
#endif
#ifdef WINDOW_RESIZEABLE
	flags |= SDL_WINDOW_RESIZABLE;
#endif
	unsigned int wWidth = 1280;
	unsigned int wHeight = 720;
	SDL_Window* wdn = SDL_CreateWindow("Engine", wWidth, wHeight, flags);

	if (wdn == nullptr)
		return -1;

	SDL_GLContext ctx = SDL_GL_CreateContext(wdn);
	if (!ctx)
		return -1;

	if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) == 0)
		return -1;

	bool running = true;
	while (running)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_EVENT_QUIT)
				running = false;
		}


		SDL_GL_SwapWindow(wdn);
	}
	return 0;
}
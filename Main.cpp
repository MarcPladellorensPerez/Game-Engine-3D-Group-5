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

	glViewport(0, 0, wWidth, wHeight);
	glClearColor(0.1f, 0.25f, 0.5f, 1.0f);
	//1. 2.
	//float v[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f };
	//float c[] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	//GLuint buffs[2];
	//glGenBuffers(2, buffs);
	//glBindBuffer(GL_ARRAY_BUFFER, buffs[0]);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
	//glBindBuffer(GL_ARRAY_BUFFER, buffs[1]);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(c), c, GL_STATIC_DRAW);

	//3.
	float vertices[] = {
	-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 
	 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 
	 0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 
	-0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f  
	};
	GLuint vBuff;
	glGenBuffers(1, &vBuff);
	glBindBuffer(GL_ARRAY_BUFFER, vBuff);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	bool running = true;
	while (running)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_EVENT_QUIT)
				running = false;
			else if (e.type == SDL_EVENT_WINDOW_RESIZED)
			{
				wWidth = e.window.data1;
				wHeight = e.window.data2;
				glViewport(0, 0, wWidth, wHeight);
			}
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//0.glBegin(GL_TRIANGLES);
		//0.glVertex3f(-0.5f, -0.5f, 0.0f);
		//0.//glVertex3fv(v);
		//0.glColor3f(1.0f, 0.0f, 0.0f);
		//0.//glColor3fv(c);
		//0.glVertex3f(0.5f, -0.5f, 0.0f);
		//0.//glVertex3fv(&v[3]);
		//0.glColor3f(0.0f, 1.0f, 0.0f);
		//0.//glColor3fv(&c[3]);
		//0.glVertex3f(0.0f, 0.5f, 0.0f);
		//0.//glVertex3fv(&v[6]);
		//0.glColor3f(0.0f, 0.0f, 1.0f);
		//0.//glColor3fv(&c[6]);
		//0.glEnd();
		//1. 2. 3.
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		//1. glVertexPointer(3, GL_FLOAT, 0, v);
		//1. glColorPointer(3, GL_FLOAT, 0, c);
		//2. glBindBuffer(GL_ARRAY_BUFFER, buffs[0]);
		//2. glVertexPointer(3, GL_FLOAT, 0, NULL);
		//2. glBindBuffer(GL_ARRAY_BUFFER, buffs[1]);
		//2. glColorPointer(3, GL_FLOAT, 0, NULL);

		//3.
		glBindBuffer(GL_ARRAY_BUFFER, vBuff);
		glVertexPointer(3, GL_FLOAT, sizeof(float) * 6, NULL);
		glColorPointer(3, GL_FLOAT, sizeof(float) * 6, reinterpret_cast<void*>(sizeof(float) * 3));

		//1. 2. 3.
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		SDL_GL_SwapWindow(wdn);
	}
	return 0;
}
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "backend.hpp"
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

class SDLBackend: public Backend {
public:
	virtual ~SDLBackend(){

	}

	virtual void init(int width, int height){
		if ( SDL_Init(SDL_INIT_VIDEO) != 0 ){
			fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
			exit(1);
		}

		SDL_SetVideoMode(width, height, 0, SDL_OPENGL|SDL_DOUBLEBUF);

		glClearColor(1,0,1,1);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, 0, height, -1.0, 1.0);
		glScalef(1, -1.0, 1);
		glTranslatef(0, -(float)height, 0);
		glMatrixMode(GL_MODELVIEW);

		glEnableClientState(GL_VERTEX_ARRAY);
	}

	virtual void poll(bool& running){
		SDL_Event event;
		while ( SDL_PollEvent(&event) ){
			switch ( event.type ){
			case SDL_KEYDOWN:
				if ( event.key.keysym.sym == SDLK_ESCAPE || (event.key.keysym.sym == SDLK_q && event.key.keysym.mod & KMOD_CTRL) ){
					running = false;
				}
				break;

			case SDL_QUIT:
				running = false;
				break;
			}
		}
	}

	virtual void cleanup(){
		SDL_Quit();
	}

	static Backend* factory(){
		return new SDLBackend;
	}

	virtual void render_begin(){
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
	}

	virtual void render_end(){
		SDL_GL_SwapBuffers();
	}

	virtual void render_tilemap(){
		static const float v[][3] = {
			{-50,-50,0},
			{50,-50,0},
			{50,50,0},
			{-50,50,0},
		};
		static const unsigned int indices[4] = {0,1,2,3};

		glColor4f(1,1,1,1);
		glVertexPointer(3, GL_FLOAT, sizeof(float)*3, v);
		glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);

		int err;
		if ( (err=glGetError()) != GL_NO_ERROR ){
			fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
			exit(1);
		}
	}
};

REGISTER_BACKEND(SDLBackend);

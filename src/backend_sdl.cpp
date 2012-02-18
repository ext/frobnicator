#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "backend.hpp"
#include "tilemap.hpp"
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

class SDLTilemap: public Tilemap {
public:
	SDLTilemap(const std::string& filename)
		: Tilemap(filename) {
	}
};

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

	Tilemap* load_tilemap(const std::string& filename){
		printf("sdl load_tilemap\n");
		return new SDLTilemap(filename);
	}

	virtual void render_begin(){
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
	}

	virtual void render_end(){
		SDL_GL_SwapBuffers();
	}

	virtual void render_tilemap(const Tilemap& tilemap){
		static const float v[][3] = {
			{0,  0,  0},
			{50, 0,  0},
			{50, 50, 0},
			{0,  50, 0},
		};
		static const unsigned int indices[4] = {0,1,2,3};
		static const float c[][4] = {
			{1,1,1,1},
			{1,0,0,1},
			{0,1,0,1},
			{0,0,1,1},
		};

		glVertexPointer(3, GL_FLOAT, sizeof(float)*3, v);
		for ( size_t i = 0; i < tilemap.size(); ++i ){
			const size_t x = i % tilemap.width();
			const size_t y = i / tilemap.width();

			glPushMatrix();

			glColor4fv(c[i%4]);
			glTranslatef(x*50.0f, y*50.0f, 0.0f);
			glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);

			glPopMatrix();
		}

		int err;
		if ( (err=glGetError()) != GL_NO_ERROR ){
			fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
			exit(1);
		}
	}
};

REGISTER_BACKEND(SDLBackend);

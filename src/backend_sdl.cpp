#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "backend.hpp"
#include "tilemap.hpp"
#include "common.hpp"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
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
		glEnable(GL_TEXTURE_2D);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, 0, height, -1.0, 1.0);
		glScalef(1, -1.0, 1);
		glTranslatef(0, -(float)height, 0);
		glMatrixMode(GL_MODELVIEW);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
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
		SDLTilemap* tilemap = new SDLTilemap(filename);
		tilemap_texture = load_texture(tilemap->texture_filename());
		fprintf(stderr, "  texture: %d\n", tilemap_texture);
		return tilemap;
	}

	GLuint load_texture(const std::string filename) {
		const char* real_filename = real_path(filename.c_str());

		/* borrowed from blueflower/opengta */

		/* Load image using SDL Image */
		SDL_Surface* surface = IMG_Load(real_filename);
		if ( !surface ){
			fprintf(stderr, "failed to load texture `%s'\n", filename.c_str());
			abort();
		}

		/* To properly support all formats the surface must be copied to a new
		 * surface with a prespecified pixel format suitable for opengl.
		 *
		 * This snippet is a slightly modified version of code posted by
		 * Sam Lantinga to the SDL mailinglist at Sep 11 2002.
		 */
		SDL_Surface* rgba_surface = SDL_CreateRGBSurface(
			SDL_SWSURFACE,
			surface->w, surface->h,
			32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000
#else
			0xFF000000,
			0x00FF0000,
			0x0000FF00,
			0x000000FF
#endif
			);

		if ( !rgba_surface ) {
			fprintf(stderr, "Failed to create RGBA surface");
			abort();
		}

		/* Save the alpha blending attributes */
		Uint32 saved_flags = surface->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
		Uint8 saved_alpha = surface->format->alpha;
		if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
			SDL_SetAlpha(surface, 0, 0);
		}

		SDL_BlitSurface(surface, 0, rgba_surface, 0);

		/* Restore the alpha blending attributes */
		if ( (saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA ) {
			SDL_SetAlpha(surface, saved_flags, saved_alpha);
		}

		/* Generate texture and copy pixels to it */
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba_surface->w, rgba_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_surface->pixels );

		SDL_FreeSurface(rgba_surface);
		SDL_FreeSurface(surface);

		return texture;
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
			{0, 0, 0},
			{1, 0, 0},
			{1, 1, 0},
			{0, 1, 0},
		};
		static const unsigned int indices[4] = {0,1,2,3};

		glPushMatrix();
		glScalef(48.0f, 48.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, tilemap_texture);
		glColor4f(1,1,1,1);
		glVertexPointer(3, GL_FLOAT, sizeof(float)*3, v);

		for ( auto tile: tilemap ){
			glPushMatrix();
			{
				glTexCoordPointer(2, GL_FLOAT, sizeof(float)*2, tile.uv);
				glTranslatef(tile.x, tile.y, 0.0f);
				glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);
			}
			glPopMatrix();
		}

		glPopMatrix();
		int err;
		if ( (err=glGetError()) != GL_NO_ERROR ){
			fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
			exit(1);
		}
	}

private:
	GLuint tilemap_texture;
};

REGISTER_BACKEND(SDLBackend);

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "backend.hpp"
#include "tilemap.hpp"
#include "game.hpp"
#include "common.hpp"
#include "entity.hpp"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <map>

static const float vertices[][5] = { /* x,y,z,u,v */
	{0, 0, 0, 0, 0},
	{1, 0, 0, 1, 0},
	{1, 1, 0, 1, 1},
	{0, 1, 0, 0, 1},
};
static const unsigned int indices[4] = {0,1,2,3};

static GLuint load_texture(const std::string filename, size_t* width, size_t* height) {
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

	if ( width  ) *width  = rgba_surface->w;
	if ( height ) *height = rgba_surface->h;

	SDL_FreeSurface(rgba_surface);
	SDL_FreeSurface(surface);

	return texture;
}

class SDLTilemap: public Tilemap {
public:
	SDLTilemap(const std::string& filename)
		: Tilemap(filename) {
	}
};

class SDLSprite: public Sprite {
public:
	SDLSprite(const std::string& filename){

	}

	GLuint texture;
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
		SDL_EnableKeyRepeat(0, 0);

		for ( bool& st: pressed ){
			st = false;
		}

		glClearColor(1,0,1,1);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, 0, height, -1.0, 1.0);
		glScalef(1, -1.0, 1);
		glTranslatef(0, -(float)height, 0);
		glMatrixMode(GL_MODELVIEW);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		tower_texture = load_texture("arrowtower.png",NULL,NULL);
		printf("tower_texture: %d\n", tower_texture);
	}

	virtual void poll(bool& running){
		SDL_Event event;
		while ( SDL_PollEvent(&event) ){
			switch ( event.type ){
			case SDL_KEYDOWN:
				if ( event.key.keysym.sym == SDLK_ESCAPE || (event.key.keysym.sym == SDLK_q && event.key.keysym.mod & KMOD_CTRL) ){
					running = false;
				}

				/* fall-through */

			case SDL_KEYUP:
				handle_keyboard(event.key.keysym.sym, event.key.state == SDL_PRESSED);
				pressed[event.key.keysym.sym] = event.key.state == SDL_PRESSED;
				break;

			case SDL_MOUSEMOTION:
				Game::motion(event.motion.x, event.motion.y);
				break;

			case SDL_MOUSEBUTTONDOWN:
				if ( event.button.button == 1){
					Game::click(event.button.x, event.button.y);
				}
				break;

			case SDL_QUIT:
				running = false;
				break;
			}
		}

		/* handle panning using keyboard */
		pan.x = pan.y = 0.0f;
		if ( pressed[SDLK_LEFT ] ) pan.x += 24.0f;
		if ( pressed[SDLK_RIGHT] ) pan.x -= 24.0f;
		if ( pressed[SDLK_UP   ] ) pan.y += 24.0f;
		if ( pressed[SDLK_DOWN ] ) pan.y -= 24.0f;

		if ( fabs(pan.x) > 0.1 || fabs(pan.y) > 0.1 ){
			Game::pan(pan.x, pan.y);
		}
	}

	void handle_keyboard(SDLKey code, bool pressed){
		switch ( code ){
		default:
			break;
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
		size_t w,h;
		tilemap_texture = load_texture(tilemap->texture_filename(), &w, &h);
		tilemap->set_dimensions(w,h);
		fprintf(stderr, "  texture: %d\n", tilemap_texture);
		return tilemap;
	}

	Sprite* load_sprite(const std::string& filename){
		/* search cache */
		auto it = sprite.find(filename);
		if ( it != sprite.end() ){
			return it->second;
		}

		/* create new sprite */
		SDLSprite* tmp = new SDLSprite(filename);
		sprite[filename] = tmp;
		return tmp;
	}

	virtual void render_begin(){
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
	}

	virtual void render_end(){
		SDL_GL_SwapBuffers();
	}

	virtual void render_tilemap(const Tilemap& tilemap, const Vector2f& camera) const {
		glPushMatrix();

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		/* tile scale */
		glScalef(48.0f, 48.0f, 1.0f);

		glBindTexture(GL_TEXTURE_2D, tilemap_texture);
		glColor4f(1,1,1,1);
		glVertexPointer(3, GL_FLOAT, sizeof(float)*5, vertices);

		for ( auto tile: tilemap ){
			glPushMatrix();
			{
				glTexCoordPointer(2, GL_FLOAT, 0, tile.uv);
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

	virtual void render_marker(const Vector2f& pos, const Vector2f& camera, const bool v[]) const {
		glPushMatrix();

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		/* position */
		glTranslatef(pos.x - 48, pos.y - 48, 0.0f);

		/* tile scale */
		glScalef(48.0f, 48.0f, 1.0f);

		glBindTexture(GL_TEXTURE_2D, 0);
		glVertexPointer(3, GL_FLOAT, sizeof(float)*5, vertices);

		for ( int y = 0; y < 2; y++ ){
			for ( int x = 0; x < 2; x++ ){
				if ( v[x+y*2] ){
					glColor4f(1,1,1,0.6);
				} else {
					glColor4f(1,0,0,0.6);
				}
				glPushMatrix();
				glTranslatef(x,y,0.0f);
				glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);
				glPopMatrix();
			}
		}

		glPopMatrix();

		int err;
		if ( (err=glGetError()) != GL_NO_ERROR ){
			fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
			exit(1);
		}
	}

	virtual void render_entities(std::vector<Entity*>& entities, const Vector2f& camera) const {
		glPushMatrix();

		/* camera */
		glTranslatef(-camera.x, -camera.y - 48*2, 0.0f);

		/* tile scale */
		glScalef(48.0f, 48.0f, 1.0f);

		glVertexPointer(3, GL_FLOAT, sizeof(float)*5, vertices);
		glTexCoordPointer(2, GL_FLOAT, sizeof(float)*5, &vertices[0][3]);

		for ( Entity* ent : entities ){
			glBindTexture(GL_TEXTURE_2D, tower_texture);
			glColor4f(1,1,1,1);
			glPushMatrix();
			glTranslatef(ent->world_pos().x, ent->world_pos().y, 0.0f);
			glScalef(2,4,1);
			glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);
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
	GLuint tower_texture;
	Vector2f pan;
	bool pressed[SDLK_LAST];

	std::map<std::string, SDLSprite*> sprite;
};

REGISTER_BACKEND(SDLBackend);

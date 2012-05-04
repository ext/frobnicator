#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* the world explodes if anything related to opengl is included before windows.h */
#ifdef WIN32
#define VC_EXTRALEAN
#define NOMINMAX
#include <Windows.h>
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

typedef struct {
	float x;
	float y;
	float z;
	float s;
	float t;
} vertex;

static const float vertices[][5] = { /* x,y,z,u,v */
	{0, 0, 0, 0, 0},
	{1, 0, 0, 1, 0},
	{1, 1, 0, 1, 1},
	{0, 1, 0, 0, 1},
};
static const unsigned int indices[4] = {0,1,2,3};

static int video_flags = SDL_OPENGL|SDL_DOUBLEBUF|SDL_RESIZABLE;

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

	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

	/* Generate texture and copy pixels to it */
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
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

		size_t w,h;
		texture = load_texture(texture_filename(), &w, &h);
		set_dimensions(w,h);

		/* four vertices per tile (@todo use *strip for less vertices) */
		vertices = (vertex*)malloc(sizeof(vertex)*4*size());
		indices = (unsigned int*)malloc(sizeof(unsigned int)*4*size());

		/* generate vertices */
		fprintf(stderr, "  generating vertices\n");
		unsigned int n = 0;
		for ( auto it = begin(); it != end(); ++it ){
			Tilemap::Tile& tile = *it;

			vertices[n  ].x = (float)((tile.x  ) * tile_width());
			vertices[n  ].y = (float)((tile.y  ) * tile_height());
			vertices[n+1].x = (float)((tile.x+1) * tile_width());
			vertices[n+1].y = (float)((tile.y  ) * tile_height());
			vertices[n+2].x = (float)((tile.x+1) * tile_width());
			vertices[n+2].y = (float)((tile.y+1) * tile_height());
			vertices[n+3].x = (float)((tile.x  ) * tile_width());
			vertices[n+3].y = (float)((tile.y+1) * tile_height());

			/* unused z */
			vertices[n  ].z = 0.0f;
			vertices[n+1].z = 0.0f;
			vertices[n+2].z = 0.0f;
			vertices[n+3].z = 0.0f;

			/* UV */
			vertices[n  ].s = tile.uv[0];
			vertices[n  ].t = tile.uv[1];
			vertices[n+1].s = tile.uv[2];
			vertices[n+1].t = tile.uv[3];
			vertices[n+2].s = tile.uv[4];
			vertices[n+2].t = tile.uv[5];
			vertices[n+3].s = tile.uv[6];
			vertices[n+3].t = tile.uv[7];

			n += 4;
		}

		/* generate indices */
		for ( unsigned int i = 0; i < 4 * size(); i++ ){
			indices[i] = i;
		}
	}

	GLuint texture;
	vertex* vertices;
	unsigned int* indices;

};

class SDLSprite: public Sprite {
public:
	SDLSprite(const std::string& filename){
		texture = load_texture(filename, &width, &height);
	}

	size_t width;
	size_t height;
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

		SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
		SDL_SetVideoMode(width, height, 0, video_flags);
		SDL_EnableKeyRepeat(0, 0);

		for ( int i = 0; i < SDLK_LAST; i++ ){
			pressed[i] = false;
		}

		glClearColor(1,0,1,1);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		setup_projection(width, height);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	void setup_projection(int w, int h){
		glViewport(0, 0, w, h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1.0, 1.0);
		glScalef(1, -1.0, 1);
		glTranslatef(0, -(float)h, 0);
		glMatrixMode(GL_MODELVIEW);
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
				Game::button_pressed(event.button.x, event.button.y, event.button.button);
				break;

			case SDL_MOUSEBUTTONUP:
				Game::button_released(event.button.x, event.button.y, event.button.button);
				break;

			case SDL_VIDEORESIZE:
				SDL_SetVideoMode(event.resize.w, event.resize.h, 0, video_flags);
				setup_projection(event.resize.w, event.resize.h);
				Game::resize(event.resize.w, event.resize.h);
				break;

			case SDL_QUIT:
				running = false;
				break;
			}
		}

		/* handle panning using keyboard */
		Vector2f pan;
		if ( pressed[SDLK_LEFT ] || pressed[SDLK_a] ) pan.x += 24.0f;
		if ( pressed[SDLK_RIGHT] || pressed[SDLK_d] ) pan.x -= 24.0f;
		if ( pressed[SDLK_UP   ] || pressed[SDLK_w] ) pan.y += 24.0f;
		if ( pressed[SDLK_DOWN ] || pressed[SDLK_s] ) pan.y -= 24.0f;

		if ( fabs(pan.x) > 0.1 || fabs(pan.y) > 0.1 ){
			Game::pan(pan.x, pan.y);
		}
	}

	void handle_keyboard(SDLKey code, bool pressed){

	}

	virtual void cleanup(){
		SDL_Quit();
	}

	static Backend* factory(){
		return new SDLBackend;
	}

	virtual Tilemap* load_tilemap(const std::string& filename){
		return new SDLTilemap(filename);
	}

	virtual Sprite* load_sprite(const std::string& filename){
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

	virtual void render_tilemap(const Tilemap& in, const Vector2f& camera) const {
		const SDLTilemap* tilemap = static_cast<const SDLTilemap*>(&in);

		glPushMatrix();

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		glBindTexture(GL_TEXTURE_2D, tilemap->texture);
		glColor4f(1,1,1,1);
		glVertexPointer(3, GL_FLOAT, sizeof(vertex), &tilemap->vertices[0].x);
		glTexCoordPointer(2, GL_FLOAT, sizeof(vertex), &tilemap->vertices[0].s);
		glDrawElements(GL_QUADS, 4*tilemap->size(), GL_UNSIGNED_INT, tilemap->indices);

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
					glColor4f(1,1,1,0.6f);
				} else {
					glColor4f(1,0,0,0.6f);
				}
				glPushMatrix();
				glTranslatef((float)x, (float)y, 0.0f);
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

		for ( auto it = entities.begin(); it != entities.end(); ++it ){
			const Entity* ent = *it;
			const SDLSprite* sprite = static_cast<const SDLSprite*>(ent->sprite());

			glBindTexture(GL_TEXTURE_2D, sprite->texture);
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
	bool pressed[SDLK_LAST];

	std::map<std::string, SDLSprite*> sprite;
};

REGISTER_BACKEND(SDLBackend);

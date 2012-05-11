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
#include "region.hpp"
#include "sprite.hpp"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <GL/glew.h>
#include <math.h>
#include <map>
#include <cassert>
#include <algorithm>

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
static const unsigned int line_indices[5] = {0, 1, 2, 3, 0};
static Vector2i size;

static int video_flags = SDL_OPENGL|SDL_DOUBLEBUF|SDL_RESIZABLE;

static GLuint load_texture(const std::string filename, size_t* width, size_t* height) {
	const char* real_filename = real_path(filename.c_str());

	/* borrowed from blueflower/opengta */

	/* Load image using SDL Image */
	SDL_Surface* surface = IMG_Load(real_filename);
	if ( !surface ){
		fprintf(stderr, "failed to load texture `%s'\n", filename.c_str());

		static const char* default_texture = "default.png";
		if ( filename == default_texture ) abort();
		return load_texture(default_texture, width, height);
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

#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
#endif

	/* Generate texture and copy pixels to it */
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba_surface->w, rgba_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_surface->pixels );

	if ( width  ) *width  = rgba_surface->w;
	if ( height ) *height = rgba_surface->h;

	SDL_FreeSurface(rgba_surface);
	SDL_FreeSurface(surface);

	return texture;
}

static void setup_projection(const Vector2i resolution){
	glViewport(0, 0, resolution.x, resolution.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, resolution.x, 0, resolution.y, -1.0, 1.0);
	glScalef(1, -1.0, 1);
	glTranslatef(0, -(float)resolution.y, 0);
	glMatrixMode(GL_MODELVIEW);
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

class SDLSprite;
static std::map<std::string, SDLSprite*> texture_cache;

class SDLSprite: public Sprite {
public:
	SDLSprite(){
	}

	virtual void load_texture(const std::string& filename){
		/* search cache */
		auto it = texture_cache.find(filename);
		if ( it != texture_cache.end() ){
			SDLSprite* c = it->second;
			texture = c->texture;
			width   = c->width;
			height  = c->height;
		}

		/* load new texture */
		texture = ::load_texture(filename, &width, &height);
		texture_cache[filename] = this;
	}

	size_t width;
	size_t height;
	GLuint texture;
};

class SDLRenderTarget: public RenderTarget {
public:
	static SDLRenderTarget* current;

	SDLRenderTarget(const Vector2i& size)
		: size(size)
		, id(0) {

		glGenFramebuffersEXT(1, &id);
		glGenTextures(1, &color);

		bind();
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		glBindTexture(GL_TEXTURE_2D, color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color, 0);

		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if(status != GL_FRAMEBUFFER_COMPLETE_EXT){
			fprintf(stderr, "Framebuffer incomplete: %s\n", gluErrorString(status));
			abort();
		}

		unbind();
	}

	virtual ~SDLRenderTarget(){
		glDeleteFramebuffersEXT(1, &id);
		glDeleteTextures(1, &color);
	}

	virtual void bind(){
		if ( current ){
			fprintf(stderr, "Nesting problem with SDLRenderTarget, did you call Backend::render_end()?\n");
			abort();
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
		current = this;

		setup_projection(size);
	}

	virtual void unbind(){
		if ( !current ){
			fprintf(stderr, "Nesting problem with SDLRenderTarget, did you call Backend::render_begin(..)?\n");
			abort();
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		current = nullptr;
	}

	const Vector2i size;
	GLuint id;
	GLuint color;
};

SDLRenderTarget* SDLRenderTarget::current = nullptr;

class SDLBackend: public Backend {
public:
	virtual ~SDLBackend(){

	}

	virtual void init(const Vector2i& size){
		::size = size;

		if ( SDL_Init(SDL_INIT_VIDEO) != 0 ){
			fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
			exit(1);
		}

		SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
		SDL_SetVideoMode(size.x, size.y, 0, video_flags);
		SDL_EnableKeyRepeat(0, 0);

		int ret;
		if ( (ret=glewInit()) != GLEW_OK ){
			fprintf(stderr, "Failed to initialize GLEW: %s\n", glewGetErrorString(ret));
			exit(1);
		}

		for ( int i = 0; i < SDLK_LAST; i++ ){
			pressed[i] = false;
			actions[i] = NULL;
		}

		glClearColor(1,0,1,1);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

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

				if ( actions[event.key.keysym.sym] ){
					actions[event.key.keysym.sym]();
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
				size = Vector2i(event.resize.w, event.resize.h);
				SDL_SetVideoMode(size.x, size.y, 0, video_flags);
				Game::resize(size);
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

	virtual void bindkey(const std::string& key, std::function<void()> func) {
		/* fulhack for the keys I actually use.... */
		     if ( key == "F1"  ){	actions[SDLK_F1 ] = func; }
		else if ( key == "F2"  ){	actions[SDLK_F2 ] = func; }
		else if ( key == "F3"  ){	actions[SDLK_F3 ] = func; }
		else if ( key == "F4"  ){	actions[SDLK_F4 ] = func; }
		else if ( key == "F5"  ){	actions[SDLK_F5 ] = func; }
		else if ( key == "F6"  ){	actions[SDLK_F6 ] = func; }
		else if ( key == "F7"  ){	actions[SDLK_F7 ] = func; }
		else if ( key == "F8"  ){	actions[SDLK_F8 ] = func; }
		else if ( key == "F9"  ){	actions[SDLK_F9 ] = func; }
		else if ( key == "F10" ){	actions[SDLK_F10] = func; }
		else if ( key == "F11" ){	actions[SDLK_F11] = func; }
		else if ( key == "F12" ){	actions[SDLK_F12] = func; }
		else if ( key == "F13" ){	actions[SDLK_F13] = func; }
		else {
			fprintf(stderr, "key '%s` not recognized.\n", key.c_str());
			abort();
		}
	}

	static Backend* factory(){
		return new SDLBackend;
	}

	virtual Tilemap* load_tilemap(const std::string& filename){
		return new SDLTilemap(filename);
	}

	virtual Sprite* create_sprite(){
		return new SDLSprite;
	}

	virtual RenderTarget* create_rendertarget(const Vector2i& size) {
		return new SDLRenderTarget(size);
	}

	virtual void render_begin(RenderTarget* target){
		if ( target ){
			target->bind();
		} else {
			setup_projection(size);
		}

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
	}

	virtual void render_end(){
		if ( SDLRenderTarget::current ){
			SDLRenderTarget::current->unbind();
			return;
		}

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
			abort();
		}
	}

	virtual void render_marker(const Vector2f& pos, const Vector2f& camera, const bool v[]) const {
		glPushMatrix();

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		/* position */
		glTranslatef(pos.x - Game::tile_width(), pos.y - Game::tile_height(), 0.0f);

		/* tile scale */
		glScalef(Game::tile_width(), Game::tile_height(), 1.0f);

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
			abort();
		}
	}

	virtual void render_region(const Region* region, const Vector2f& camera, float color[3]) const {
		glPushMatrix();

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		/* position */
		glTranslatef(region->x(), region->y(), 0.0f);

		/* tile scale */
		glScalef(region->w(), region->h(), 1.0f);

		glBindTexture(GL_TEXTURE_2D, 0);
		glVertexPointer(3, GL_FLOAT, sizeof(float)*5, vertices);

		glColor4f(color[0], color[1], color[2], 0.5f);
		glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);

		/* outline */
		glPushAttrib (GL_LINE_BIT);
		glLineWidth(2);
		glColor4f(color[0], color[1], color[2], 1.0f);
		glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_INT, line_indices);
		glPopAttrib();

		glPopMatrix();

		int err;
		if ( (err=glGetError()) != GL_NO_ERROR ){
			fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
			abort();
		}
	}

	virtual void render_region(const Entity* ent, const Vector2f& camera, float color[3]) const {
		const SDLSprite* sprite = static_cast<const SDLSprite*>(ent->sprite());

		glPushMatrix();

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		/* position */
		glTranslatef(ent->world_pos().x, ent->world_pos().y, 0.0f);

		/* tile scale */
		glScalef(Game::tile_width() * sprite->scale().x, Game::tile_height() * (sprite->scale().y+sprite->offset().y), 1.0f);

		glBindTexture(GL_TEXTURE_2D, 0);
		glVertexPointer(3, GL_FLOAT, sizeof(float)*5, vertices);

		glColor4f(color[0], color[1], color[2], 0.5f);
		glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);

		/* outline */
		glPushAttrib (GL_LINE_BIT);
		glLineWidth(2);
		glColor4f(color[0], color[1], color[2], 1.0f);
		glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_INT, line_indices);
		glPopAttrib();

		glPopMatrix();

		int err;
		if ( (err=glGetError()) != GL_NO_ERROR ){
			fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
			abort();
		}
	}

	virtual void render_entities(std::vector<Entity*>& entities, const Vector2f& camera) const {
		glPushMatrix();
		glPushAttrib(GL_ENABLE_BIT);

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		glVertexPointer(3, GL_FLOAT, sizeof(float)*5, vertices);
		glTexCoordPointer(2, GL_FLOAT, sizeof(float)*5, &vertices[0][3]);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		for ( auto it = entities.begin(); it != entities.end(); ++it ){
			const Entity* ent = *it;
			const SDLSprite* sprite = static_cast<const SDLSprite*>(ent->sprite());
			assert(sprite);

			glEnable(GL_TEXTURE_2D);
			glColor4f(1,1,1,1);
			glBindTexture(GL_TEXTURE_2D, sprite->texture);

			glPushMatrix();
			glTranslatef(
				ent->world_pos().x + Game::tile_width()  * sprite->offset().x,
				ent->world_pos().y + Game::tile_height() * sprite->offset().y,
				0.0f);

			/* entity scale */
			glPushMatrix();
			glScalef(
				Game::tile_width()  * sprite->scale().x,
				Game::tile_height() * sprite->scale().y,
				1.0f);

			glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);

			glPopMatrix(); /* revert scale */

			/* Render healthbar */
			const float s = ent->current_hp() / ent->max_hp();
			if ( s < 1.0f ){
				const float w = Game::tile_width()  * sprite->scale().x * s;
				glDisable(GL_TEXTURE_2D);
				glColor4f(1.0f - s, s, 0.0f, 1.0f);
				glBegin(GL_QUADS);
				glVertex2f(0.0f, -10.0f);
				glVertex2f(w,    -10.0f);
				glVertex2f(w,    - 3.0f);
				glVertex2f(0.0f, - 3.0f);
				glEnd();
			}

			glPopMatrix();
		}

		glPopAttrib();
		glPopMatrix();

		int err;
		if ( (err=glGetError()) != GL_NO_ERROR ){
			fprintf(stderr, "OpenGL error: %s\n", gluErrorString(err));
			abort();
		}
	}

	virtual void render_projectiles(std::vector<Projectile*>& projectiles, const Vector2f& camera) const {
		glPushMatrix();
		glPushAttrib(GL_ENABLE_BIT);

		/* camera */
		glTranslatef(-camera.x, -camera.y, 0.0f);

		glColor4f(1,1,1,1);
		glDisable(GL_TEXTURE_2D);
		std::for_each(projectiles.begin(), projectiles.end(), [](const Projectile* proj){
			Vector2f src, dst;
			proj->get_points(&src, &dst);

			glBegin(GL_LINES);
			glVertex2f(src.x, src.y);
			glVertex2f(dst.x, dst.y);
			glEnd();
		});

		glPopAttrib();
		glPopMatrix();
	}

	virtual void render_target(RenderTarget* in_target, const Vector2i& offset) const {
		SDLRenderTarget* target = static_cast<SDLRenderTarget*>(in_target);

		glPushMatrix();

		glColor4f(1,1,1,1);
		glBindTexture(GL_TEXTURE_2D, target->color);

		const Vector2i real_offset(
			offset.x >= 0 ? offset.x : size.x + offset.x,
			offset.y >= 0 ? offset.y : size.y + offset.y
		);

		glTranslatef(real_offset.x, target->size.y + real_offset.y, 0.0f);
		glScalef(target->size.x, -target->size.y, 1.0f);
		glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indices);

		glPopMatrix();
	}

private:
	bool pressed[SDLK_LAST];
	std::function<void()> actions[SDLK_LAST];
};

REGISTER_BACKEND(SDLBackend);

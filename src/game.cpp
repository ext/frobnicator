#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include "backend.hpp"
#include "level.hpp"
#include "tilemap.hpp"
#include <cstdlib>
#include <cassert>
#include <vector>

typedef std::vector<Entity*> EntityVector;

static bool running = false;
static Backend* backend = NULL;
static Level* level = NULL;
static EntityVector entity;
static Vector2f camera;
static Tilemap* tilemap;
static int width;
static int height;

static void poll(bool&render){
	backend->poll(running);
}

static void render_game(){
	backend->render_begin();
	{
		backend->render_tilemap(level->tilemap(), camera);

	}
	backend->render_end();
}

namespace Game {
	void init(const std::string& bn, int w, int h){
		width = w;
		height = h;
		backend = Backend::create(bn);

		if ( !backend ){
			fprintf(stderr, "Failed to create backend `%s'.\n", bn.c_str());
			exit(1);
		}

		backend->init(width, height);
	}

	void cleanup(){
		backend->cleanup();
		delete backend;
	}

	void frobnicate(){
		running = true;
		while ( running ){
			poll(running); /* byref */
			render_game();
		}
	}

	void load_level(const std::string& filename){
		delete level;
		level = Level::from_filename(filename);
	}

	Tilemap* load_tilemap(const std::string& filename){
		assert(backend);
		tilemap = backend->load_tilemap(filename);
		return tilemap;
	}

	void pan(float x, float y){
		const float bx = tilemap->tile_width()  * tilemap->map_width()  - width;
		const float by = tilemap->tile_height() * tilemap->map_height() - height;

		camera.x = clamp(camera.x - x, 0.0f, bx);
		camera.y = clamp(camera.y - y, 0.0f, by);
	}

	void motion(float x, float y){
	}
};

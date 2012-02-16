#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include "backend.hpp"
#include "level.hpp"
#include <cstdlib>
#include <cassert>
#include <vector>

typedef std::vector<Entity*> EntityVector;

static bool running = false;
static Backend* backend = NULL;
static Level* level = NULL;
static EntityVector entity;

static void poll(bool&render){
	backend->poll(running);
}

static void render_game(){
	backend->render_begin();
	{
		backend->render_tilemap(level->tilemap());

	}
	backend->render_end();
}

namespace Game {
	void init(const std::string& bn, int width, int height){
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
		printf("load_tilemap\n");
		return backend->load_tilemap(filename);
	}
};

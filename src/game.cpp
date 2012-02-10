#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include "backend.hpp"
#include "level.hpp"
#include <cstdlib>
#include <vector>

typedef std::vector<Entity*> EntityVector;

class GamePimpl {
public:
	void frobnicate(){
		backend->init(800, 600);

		running = true;
		while ( running ){
			poll(running);
			render_game();
		}

		backend->cleanup();
	}

	void poll(bool&render){
		backend->poll(running);
	}

	void render_game(){
		backend->render_begin();
		{
			backend->render_tilemap();

		}
		backend->render_end();
	}

	bool running;
	Backend* backend;
	Level* level;
	EntityVector entity;
};

Game::Game()
	: pimpl(new GamePimpl){

	pimpl->backend = Backend::create("SDLBackend");
	if ( !pimpl->backend ){
		fprintf(stderr, "No backend.\n");
		exit(1);
	}
}

Game::~Game(){
	delete pimpl->level;
	delete pimpl;
}

void Game::load_level(const std::string& filename){
	pimpl->level = new Level(filename);
}

bool Game::running() const {
	return pimpl->running;
}

void Game::frobnicate(){
	pimpl->frobnicate();
}

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include <cstdio>
#include <cstring>

int main(int argc, const char* argv[]){
	std::string filename = "maul.level";

	if ( argc >= 2 ){
		filename = argv[1];
	}

	Game::init("SDLBackend", 800, 600);
	Game::load_level(filename);
	Game::frobnicate();
	Game::cleanup();

	return 0;
}

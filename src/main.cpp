#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game.hpp"
#include <cstdio>
#include <cstring>

static const char* datapath(){
	const char* path = getenv("DATA_DIR");
	if ( !path ){
		path = DATA_DIR;
	}
	return path;
}

/**
 * Expands the path to the data-directory. Returns memory to static memory which
 * will be overwritten between successive calls.
 */
const char* real_path(const char* filename){
	static char buffer[4096];

	if ( filename[0] == '/' || filename[1] == ':' ){
		return filename;
	}

	if ( snprintf(buffer, 4096, "%s/%s", datapath(), filename) == -1 ){
		abort();
	}

	return buffer;
}

int main(int argc, const char* argv[]){
	/* default level */
	std::string filename = "mail.level";

	if ( argc >= 2 ){
		filename = argv[1];
	}

	Game game;
	game.load_level("maul.level");
	game.frobnicate();

	return 0;
}

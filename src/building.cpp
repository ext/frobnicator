#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "building.hpp"
#include "creep.hpp"
#include "game.hpp"
#include <sstream>

Building::Building(const Vector2f& pos, const Blueprint* blueprint)
	: Entity(generate_id(), pos, blueprint, 0) {

	fprintf(stderr, "Creating \"%s\" at (%.0f,%.0f)\n", name().c_str(), pos.x, pos.y);
}

const std::string Building::generate_id(){
	static int n = 1;
	std::stringstream s;
	s << "building_" << n++;
	return s.str();
}

void Building::tick(float dt){
	if ( can_fire() ){
		for ( auto it = Game::all_creep().begin(); it != Game::all_creep().end(); ++it ){
			Creep* creep = it->second;
			const float distance = (world_pos() - creep->world_pos()).length();

			if ( distance < range() ){
				fprintf(stderr, "Entity `%s' can fire at `%s'.\n", id().c_str(), creep->id().c_str());
				break;
			}
		}
	}
}

bool Building::can_fire() const {
	return true;
}

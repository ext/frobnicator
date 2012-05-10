#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "building.hpp"
#include "creep.hpp"
#include "game.hpp"
#include <sstream>
#include <time.h>
#include <sys/time.h>

Building::Building(const Vector2f& pos, const Blueprint* blueprint)
	: Entity(generate_id(), pos, blueprint, 0)
	, last_firing(0) {

	fprintf(stderr, "Creating \"%s\" at (%.0f,%.0f)\n", name().c_str(), pos.x, pos.y);
	firing_delta = static_cast<uint64_t>(600.0f / rof()); /* convert shots/min to deciseconds */
	fprintf(stderr, "rof: %f delta: %lu\n", rof(), firing_delta);
}

const std::string Building::generate_id(){
	static int n = 1;
	std::stringstream s;
	s << "building_" << n++;
	return s.str();
}

void Building::tick(float dt){
	Creep* t = have_target() ? dynamic_cast<Creep*>(Game::find_entity(target)) : NULL;

	if ( t ){
		const float distance = (world_pos() - t->world_pos()).length();
		if ( distance > range() ){
			target = "";
		}
	}

	/* test if it can fire */
	if ( t && can_fire() ){
		fire_at(t);		
	}

	if ( !t ){
		for ( auto it = Game::all_creep().begin(); it != Game::all_creep().end(); ++it ){
			Creep* creep = it->second;
			const float distance = (world_pos() - creep->world_pos()).length();

			if ( distance < range() ){
				target = creep->id();
				break;
			}
		}
	}
}

bool Building::can_fire() const {
	struct timeval t;
	gettimeofday(&t, NULL);

	uint64_t cur = t.tv_sec * 10 + t.tv_usec / 100000;
	uint64_t delta = cur - last_firing;
	return delta >= firing_delta;
}

bool Building::have_target() const {
	return target != "";
}

void Building::fire_at(Creep* creep){
	fprintf(stderr, "`%s' fires at `%s'.\n", id().c_str(), creep->id().c_str());
	creep->damage(damage());

	struct timeval t;
	gettimeofday(&t, NULL);
	last_firing = t.tv_sec * 10 + t.tv_usec / 100000;
}

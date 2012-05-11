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

	firing_delta = static_cast<uint64_t>(600.0f / rof()); /* convert shots/min to deciseconds */
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
		/* find closest entity within range */
		float current = range();

		for ( auto it = Game::all_creep().begin(); it != Game::all_creep().end(); ++it ){
			Creep* creep = it->second;
			const float distance = Vector2f::distance(world_pos(), creep->world_pos());

			if ( distance < current ){
				target = creep->id();
				current = distance;
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
	/* Projectile constructor has side-effects, will deallocate itself when hit. */
	new Projectile(world_pos() + Vector2f(48.0f, -24.0f), creep, 700.0f, 25.0f, [creep, this](){
		creep->damage(damage());
	});

	struct timeval t;
	gettimeofday(&t, NULL);
	last_firing = t.tv_sec * 10 + t.tv_usec / 100000;
}

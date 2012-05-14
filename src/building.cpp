#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "building.hpp"
#include "blueprint.hpp"
#include "creep.hpp"
#include "game.hpp"
#include <sstream>
#include <time.h>
#include <sys/time.h>

Building::Building(const Vector2f& pos, const Blueprint* blueprint)
	: Entity(generate_id(), pos, blueprint, 1)
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

		/* reset target for towers with buffs */
		if ( have_slow() || have_poison() ){
			target = "";
		}
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

bool Building::have_slow() const {
	return slow() > 0.0f && slow_duration() > 0.0f;
}

bool Building::have_poison() const {
	return poison() > 0.0f && poison_duration() > 0.0f;
}

bool Building::have_target() const {
	return target != "";
}

SlowBuff Building::slow_buff() const {
	return SlowBuff(slow(), slow_duration());
}

PoisonBuff Building::poison_buff() const {
	return PoisonBuff(poison(), poison_duration());
}

bool Building::can_upgrade() const {
	/* -1 because first is base stat */
	return current_level() < (int)blueprint->num_levels() - 1;
}

int Building::upgrade_cost() const {
	return blueprint->cost(current_level()+1);
}

int Building::sell_cost() const {
	int sum = 0;
	int n = level;
	while ( n > 0 ){
		sum += blueprint->cost(n);
		n--;
	}
	return (int)(sum * 0.75f);
}

void Building::upgrade(){
	if ( Game::transaction(upgrade_cost(), world_pos()) ){
		level++;
		firing_delta = static_cast<uint64_t>(600.0f / rof()); /* convert shots/min to deciseconds */
	}
}

void Building::sell(){
	Game::transaction(-sell_cost(), world_pos());
	Game::remove_entity(id());
}

void Building::fire_at(Creep* creep){
	/* must increase reference count because projectile callback requires the instance. */
	inc_ref();

	/* Projectile constructor has side-effects, will deallocate itself when hit. */
	new Projectile(world_pos() + Vector2f(48.0f, -24.0f), creep, 700.0f, 25.0f, [creep, this](){
		creep->damage(damage(), this);

		if ( have_slow()   ){ creep->add_buff(slow_buff()); }
		if ( have_poison() ){ creep->add_buff(poison_buff()); }

		dec_ref();
	});

	struct timeval t;
	gettimeofday(&t, NULL);
	last_firing = t.tv_sec * 10 + t.tv_usec / 100000;
}

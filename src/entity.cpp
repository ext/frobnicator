#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "entity.hpp"
#include "blueprint.hpp"
#include "creep.hpp"
#include "game.hpp"
#include "waypoint.hpp"
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <yaml.h>

#ifdef WIN32
#define strdup _strdup
extern "C" char* strndup(const char* src, size_t n);
#endif

Entity::Entity(const std::string& id, const Vector2f& pos, const Blueprint* blueprint, unsigned int level)
	: level(level)
	, pos(pos)
	, _id(id)
	, blueprint(blueprint) {

}

const Vector2f& Entity::world_pos() const {
	return pos;
}

const Vector2i Entity::grid_pos() const {
	return Vector2i(pos.x / Game::tile_width(), pos.y / Game::tile_height());
}

const Sprite* Entity::sprite() const {
	return blueprint->sprite(level);
}

const std::string Entity::name() const {
	return blueprint->name(level);
}

const std::string Entity::id() const {
	return _id;
}

int Entity::cost()     const { return blueprint->data[level].cost; }
float Entity::splash() const { return blueprint->data[level].splash; }
float Entity::damage() const { return blueprint->data[level].damage; }
float Entity::rof()    const { return blueprint->data[level].rof; }
float Entity::range()  const { return blueprint->data[level].range; }
float Entity::slow()   const { return blueprint->data[level].slow; }
float Entity::poison() const { return blueprint->data[level].poison; }
float Entity::speed()  const { return blueprint->data[level].speed; }
float Entity::armor()  const { return blueprint->data[level].armor; }

void Entity::kill(){
	fprintf(stderr, "Entity %s was killed\n", id().c_str());
	Game::remove_entity(id());
}

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
